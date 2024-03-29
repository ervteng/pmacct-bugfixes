/*
    pmacct (Promiscuous mode IP Accounting package)
    pmacct is Copyright (C) 2003-2014 by Paolo Lucente
*/

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#define __AMQP_PLUGIN_C

/* includes */
#include "pmacct.h"
#include "pmacct-data.h"
#include "plugin_hooks.h"
#include "plugin_common.h"
#include "amqp_plugin.h"
#include "ip_flow.h"
#include "classifier.h"
#include "crc32.c"

/* Functions */
void amqp_plugin(int pipe_fd, struct configuration *cfgptr, void *ptr) 
{
  struct pkt_data *data;
  struct ports_table pt;
  unsigned char *pipebuf;
  struct pollfd pfd;
  time_t t, now;
  int timeout, ret, num; 
  struct ring *rg = &((struct channels_list_entry *)ptr)->rg;
  struct ch_status *status = ((struct channels_list_entry *)ptr)->status;
  int datasize = ((struct channels_list_entry *)ptr)->datasize;
  u_int32_t bufsz = ((struct channels_list_entry *)ptr)->bufsize;
  struct networks_file_data nfd;

  unsigned char *rgptr;
  int pollagain = TRUE;
  u_int32_t seq = 1, rg_err_count = 0;

  struct extra_primitives extras;
  struct primitives_ptrs prim_ptrs;
  char *dataptr;

  memcpy(&config, cfgptr, sizeof(struct configuration));
  memcpy(&extras, &((struct channels_list_entry *)ptr)->extras, sizeof(struct extra_primitives));
  recollect_pipe_memory(ptr);
  pm_setproctitle("%s [%s]", "RabbitMQ/AMQP Plugin", config.name);

  P_set_signals();
  P_init_default_values();
  pipebuf = (unsigned char *) Malloc(config.buffer_size);
  memset(pipebuf, 0, config.buffer_size);

  timeout = config.sql_refresh_time*1000;

  if (!config.sql_user) config.sql_user = rabbitmq_user;
  if (!config.sql_passwd) config.sql_passwd = rabbitmq_pwd;

  /* setting function pointers */
  if (config.what_to_count & (COUNT_SUM_HOST|COUNT_SUM_NET))
    insert_func = P_sum_host_insert;
  else if (config.what_to_count & COUNT_SUM_PORT) insert_func = P_sum_port_insert;
  else if (config.what_to_count & COUNT_SUM_AS) insert_func = P_sum_as_insert;
#if defined (HAVE_L2)
  else if (config.what_to_count & COUNT_SUM_MAC) insert_func = P_sum_mac_insert;
#endif
  else insert_func = P_cache_insert;
  purge_func = amqp_cache_purge;

  memset(&nt, 0, sizeof(nt));
  memset(&nc, 0, sizeof(nc));
  memset(&pt, 0, sizeof(pt));

  load_networks(config.networks_file, &nt, &nc);
  set_net_funcs(&nt);

  if (config.ports_file) load_ports(config.ports_file, &pt);
  if (config.pkt_len_distrib_bins_str) load_pkt_len_distrib_bins();
  else {
    if (config.what_to_count_2 & COUNT_PKT_LEN_DISTRIB) {
      Log(LOG_ERR, "ERROR ( %s/%s ): 'aggregate' contains pkt_len_distrib but no 'pkt_len_distrib_bins' defined. Exiting.\n", config.name, config.type);
      exit_plugin(1);
    }
  }
  
  memset(&prim_ptrs, 0, sizeof(prim_ptrs));
  set_primptrs_funcs(&extras);

  pfd.fd = pipe_fd;
  pfd.events = POLLIN;
  setnonblocking(pipe_fd);

  now = time(NULL);

  /* print_refresh time init: deadline */
  refresh_deadline = now; 
  t = roundoff_time(refresh_deadline, config.sql_history_roundoff);
  while ((t+config.sql_refresh_time) < refresh_deadline) t += config.sql_refresh_time;
  refresh_deadline = t;
  refresh_deadline += config.sql_refresh_time; /* it's a deadline not a basetime */

  if (config.sql_history) {
    basetime_init = P_init_historical_acct;
    basetime_eval = P_eval_historical_acct;
    basetime_cmp = P_cmp_historical_acct;

    (*basetime_init)(now);
  }

  /* setting number of entries in _protocols structure */
  while (_protocols[protocols_number].number != -1) protocols_number++;

  /* plugin main loop */
  for(;;) {
    poll_again:
    status->wakeup = TRUE;
    ret = poll(&pfd, 1, timeout);

    if (ret <= 0) {
      if (getppid() == 1) {
        Log(LOG_ERR, "ERROR ( %s/%s ): Core process *seems* gone. Exiting.\n", config.name, config.type);
        exit_plugin(1);
      }

      if (ret < 0) goto poll_again;
    }

    now = time(NULL);

    if (config.sql_history) {
      while (now > (basetime.tv_sec + timeslot)) {
	new_basetime.tv_sec = basetime.tv_sec;
        basetime.tv_sec += timeslot;
        if (config.sql_history == COUNT_MONTHLY)
          timeslot = calc_monthly_timeslot(basetime.tv_sec, config.sql_history_howmany, ADD);
      }
    }

    switch (ret) {
    case 0: /* timeout */
      P_cache_handle_flush_event(&pt);
      break;
    default: /* we received data */
      read_data:
      if (!pollagain) {
        seq++;
        seq %= MAX_SEQNUM;
        if (seq == 0) rg_err_count = FALSE;
      }
      else {
        if ((ret = read(pipe_fd, &rgptr, sizeof(rgptr))) == 0) 
	  exit_plugin(1); /* we exit silently; something happened at the write end */
      }

      if (((struct ch_buf_hdr *)rg->ptr)->seq != seq) {
        if (!pollagain) {
          pollagain = TRUE;
          goto poll_again;
        }
        else {
          rg_err_count++;
          if (config.debug || (rg_err_count > MAX_RG_COUNT_ERR)) {
            Log(LOG_ERR, "ERROR ( %s/%s ): We are missing data.\n", config.name, config.type);
            Log(LOG_ERR, "If you see this message once in a while, discard it. Otherwise some solutions follow:\n");
            Log(LOG_ERR, "- increase shared memory size, 'plugin_pipe_size'; now: '%u'.\n", config.pipe_size);
            Log(LOG_ERR, "- increase buffer size, 'plugin_buffer_size'; now: '%u'.\n", config.buffer_size);
            Log(LOG_ERR, "- increase system maximum socket size.\n\n");
          }
          seq = ((struct ch_buf_hdr *)rg->ptr)->seq;
        }
      }

      pollagain = FALSE;
      memcpy(pipebuf, rg->ptr, bufsz);
      if ((rg->ptr+bufsz) >= rg->end) rg->ptr = rg->base;
      else rg->ptr += bufsz;

      /* lazy refresh time handling */ 
      if (now > refresh_deadline) P_cache_handle_flush_event(&pt);

      data = (struct pkt_data *) (pipebuf+sizeof(struct ch_buf_hdr));

      while (((struct ch_buf_hdr *)pipebuf)->num > 0) {
        for (num = 0; primptrs_funcs[num]; num++)
          (*primptrs_funcs[num])((u_char *)data, &extras, &prim_ptrs);

	for (num = 0; net_funcs[num]; num++)
	  (*net_funcs[num])(&nt, &nc, &data->primitives, prim_ptrs.pbgp, &nfd);

	if (config.ports_file) {
          if (!pt.table[data->primitives.src_port]) data->primitives.src_port = 0;
          if (!pt.table[data->primitives.dst_port]) data->primitives.dst_port = 0;
        }

        if (config.pkt_len_distrib_bins_str &&
            config.what_to_count_2 & COUNT_PKT_LEN_DISTRIB)
          evaluate_pkt_len_distrib(data);

        prim_ptrs.data = data;
        (*insert_func)(&prim_ptrs);

	((struct ch_buf_hdr *)pipebuf)->num--;
        if (((struct ch_buf_hdr *)pipebuf)->num) {
          dataptr = (unsigned char *) data;
	  dataptr += datasize;
          data = (struct pkt_data *) dataptr;
	}
      }
      goto read_data;
    }
  }
}

void amqp_cache_purge(struct chained_cache *queue[], int index)
{
  struct pkt_primitives *data = NULL;
  struct pkt_bgp_primitives *pbgp = NULL;
  struct pkt_nat_primitives *pnat = NULL;
  struct pkt_mpls_primitives *pmpls = NULL;
  char *pcust = NULL;
  struct pkt_bgp_primitives empty_pbgp;
  struct pkt_nat_primitives empty_pnat;
  struct pkt_mpls_primitives empty_pmpls;
  char *empty_pcust = NULL;
  struct amqp_basic_properties_t_ amqp_msg_props;
  char src_mac[18], dst_mac[18], src_host[INET6_ADDRSTRLEN], dst_host[INET6_ADDRSTRLEN], ip_address[INET6_ADDRSTRLEN];
  char rd_str[SRVBUFLEN], misc_str[SRVBUFLEN], dyn_amqp_routing_key[SRVBUFLEN], *orig_amqp_routing_key = NULL;
  char default_amqp_exchange[] = "pmacct", default_amqp_exchange_type[] = "direct";
  char default_amqp_routing_key[] = "acct";
  int i, j, amqp_status, batch_idx, is_routing_key_dyn = FALSE;
  time_t start, duration;
  pid_t writer_pid = getpid();

  amqp_connection_state_t amqp_conn;
  amqp_socket_t *amqp_socket = NULL;
  amqp_rpc_reply_t amqp_ret;

  /* setting some defaults */
  if (!config.sql_db) config.sql_db = default_amqp_exchange;
  if (!config.sql_table) config.sql_table = default_amqp_routing_key;
  else {
    if (strchr(config.sql_table, '$')) {
      is_routing_key_dyn = TRUE;
      orig_amqp_routing_key = config.sql_table;
      config.sql_table = dyn_amqp_routing_key;
    }
  }
  if (!config.amqp_exchange_type) config.amqp_exchange_type = default_amqp_exchange_type;

  empty_pcust = malloc(config.cpptrs.len);
  if (!empty_pcust) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Unable to malloc() empty_pcust. Exiting.\n", config.name, config.type);
    exit_plugin(1);
  }

  memset(&empty_pbgp, 0, sizeof(struct pkt_bgp_primitives));
  memset(&empty_pnat, 0, sizeof(struct pkt_nat_primitives));
  memset(&empty_pmpls, 0, sizeof(struct pkt_mpls_primitives));
  memset(empty_pcust, 0, config.cpptrs.len);
  memset(&amqp_msg_props, 0, sizeof(amqp_msg_props));

  amqp_conn = amqp_new_connection();

  amqp_socket = amqp_tcp_socket_new(amqp_conn);
  if (!socket) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: no socket\n", config.name, config.type);
    return;
  }

  if (config.sql_host)
    amqp_status = amqp_socket_open(amqp_socket, config.sql_host, 5672/* default port */);
  else 
    amqp_status = amqp_socket_open(amqp_socket, "127.0.0.1", 5672 /* default port */);

  if (amqp_status) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: unable to open socket\n", config.name, config.type);
    return;
  }

  amqp_ret = amqp_login(amqp_conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, config.sql_user, config.sql_passwd);
  if (amqp_ret.reply_type != AMQP_RESPONSE_NORMAL) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: login\n", config.name, config.type);
    return;
  }

  amqp_channel_open(amqp_conn, 1);

  amqp_ret = amqp_get_rpc_reply(amqp_conn);
  if (amqp_ret.reply_type != AMQP_RESPONSE_NORMAL) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: unable to open channel\n", config.name, config.type);
    return;
  }

  amqp_exchange_declare(amqp_conn, 1, amqp_cstring_bytes(config.sql_db), amqp_cstring_bytes(config.amqp_exchange_type), 0, 0, amqp_empty_table);
  amqp_ret = amqp_get_rpc_reply(amqp_conn);
  if (amqp_ret.reply_type != AMQP_RESPONSE_NORMAL) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: exchange declare\n", config.name, config.type);
    return;
  }

  if (config.amqp_persistent_msg) {
    amqp_msg_props._flags = (AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG);
    amqp_msg_props.content_type = amqp_cstring_bytes("text/json");
    amqp_msg_props.delivery_mode = 2; /* persistent delivery */
  }

  Log(LOG_INFO, "INFO ( %s/%s ): *** Purging cache - START (PID: %u) ***\n", config.name, config.type, writer_pid);
  start = time(NULL);

  for (j = 0; j < index; j++) {
    char *json_str;

    data = &queue[j]->primitives;
    if (queue[j]->pbgp) pbgp = queue[j]->pbgp;
    else pbgp = &empty_pbgp;

    if (queue[j]->pnat) pnat = queue[j]->pnat;
    else pnat = &empty_pnat;

    if (queue[j]->pmpls) pmpls = queue[j]->pmpls;
    else pmpls = &empty_pmpls;

    if (queue[j]->pcust) pcust = queue[j]->pcust;
    else pcust = empty_pcust;

    if (queue[j]->valid == PRINT_CACHE_FREE) continue;

    json_str = compose_json(config.what_to_count, config.what_to_count_2, queue[j]->flow_type,
                         &queue[j]->primitives, pbgp, pnat, pmpls, pcust, queue[j]->bytes_counter,
			 queue[j]->packet_counter, queue[j]->flow_counter, queue[j]->tcp_flags,
			 &queue[j]->basetime);

    if (json_str) {
      if (is_routing_key_dyn) amqp_handle_routing_key_dyn_strings(config.sql_table, SRVBUFLEN, orig_amqp_routing_key,
								  queue[j]);  

      if (config.debug) Log(LOG_DEBUG, "DEBUG ( %s/%s ): publishing [E=%s RK=%s DM=%u]: %s\n", config.name,
                            config.type, config.sql_db, config.sql_table, amqp_msg_props.delivery_mode, json_str);

      amqp_basic_publish(amqp_conn, 1, amqp_cstring_bytes(config.sql_db), amqp_cstring_bytes(config.sql_table),
			 0, 0, &amqp_msg_props, amqp_cstring_bytes(json_str));

      amqp_ret = amqp_get_rpc_reply(amqp_conn);
      if (amqp_ret.reply_type != AMQP_RESPONSE_NORMAL) {
	Log(LOG_ERR, "ERROR ( %s/%s ): Connection failed to RabbitMQ: publishing\n", config.name, config.type);
	return;
      } 

      free(json_str);
    }
  }

  amqp_channel_close(amqp_conn, 1, AMQP_REPLY_SUCCESS);
  amqp_connection_close(amqp_conn, AMQP_REPLY_SUCCESS);
  amqp_destroy_connection(amqp_conn);

  duration = time(NULL)-start;
  Log(LOG_INFO, "INFO ( %s/%s ): *** Purging cache - END (PID: %u, QN: %u, ET: %u) ***\n",
		config.name, config.type, writer_pid, index, duration);

  if (config.sql_trigger_exec) P_trigger_exec(config.sql_trigger_exec); 
}

void amqp_handle_routing_key_dyn_strings(char *new, int newlen, char *old, struct chained_cache *elem)
{
  int oldlen, ptr_len;
  char peer_src_ip_string[] = "$peer_src_ip", post_tag_string[] = "$post_tag";
  char pre_tag_string[] = "$pre_tag";
  char *ptr_start, *ptr_end;

  oldlen = strlen(old);
  if (oldlen <= newlen) strcpy(new, old);
  else {
    strncpy(new, old, newlen);
    return;
  }

  if (!strchr(new, '$')) return;
  ptr_start = strstr(new, peer_src_ip_string);
  if (ptr_start) {
    char ip_address[INET6_ADDRSTRLEN];
    char buf[newlen];
    int len;

    if (!elem || !elem->pbgp) goto out_peer_src_ip; 

    len = strlen(ptr_start);
    ptr_end = ptr_start;
    ptr_len = strlen(peer_src_ip_string);
    ptr_end += ptr_len;
    len -= ptr_len;

    addr_to_str(ip_address, &elem->pbgp->peer_src_ip);
    snprintf(buf, newlen, "%s", ip_address);
    strncat(buf, ptr_end, len);

    len = strlen(buf);
    *ptr_start = '\0';
    strncat(new, buf, len);
  }
  out_peer_src_ip:

  if (!strchr(new, '$')) return;
  ptr_start = strstr(new, post_tag_string);
  if (ptr_start) {
    char buf[newlen];
    int len;

    len = strlen(ptr_start);
    ptr_end = ptr_start;
    ptr_len = strlen(post_tag_string);
    ptr_end += ptr_len;
    len -= ptr_len;

    snprintf(buf, newlen, "%u", config.post_tag);
    strncat(buf, ptr_end, len);

    len = strlen(buf);
    *ptr_start = '\0';
    strncat(new, buf, len);
  }

  if (!strchr(new, '$')) return;
  ptr_start = strstr(new, pre_tag_string);
  if (ptr_start) {
    char buf[newlen];
    int len;

    len = strlen(ptr_start);
    ptr_end = ptr_start;
    ptr_len = strlen(pre_tag_string);
    ptr_end += ptr_len;
    len -= ptr_len;

    snprintf(buf, newlen, "%u", elem->primitives.tag);
    strncat(buf, ptr_end, len);

    len = strlen(buf);
    *ptr_start = '\0';
    strncat(new, buf, len);
  }
}
