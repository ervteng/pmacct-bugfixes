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

#define __PRINT_PLUGIN_C

/* includes */
#include "pmacct.h"
#include "pmacct-data.h"
#include "plugin_hooks.h"
#include "plugin_common.h"
#include "print_plugin.h"
#include "ip_flow.h"
#include "classifier.h"
#include "crc32.c"

/* Functions */
void print_plugin(int pipe_fd, struct configuration *cfgptr, void *ptr) 
{
  struct pkt_data *data;
  struct ports_table pt;
  unsigned char *pipebuf;
  struct pollfd pfd;
  time_t t, now;
  int timeout, ret, num, is_event;
  struct ring *rg = &((struct channels_list_entry *)ptr)->rg;
  struct ch_status *status = ((struct channels_list_entry *)ptr)->status;
  int datasize = ((struct channels_list_entry *)ptr)->datasize;
  u_int32_t bufsz = ((struct channels_list_entry *)ptr)->bufsize;
  struct networks_file_data nfd;
  char default_separator[] = ",";

  unsigned char *rgptr;
  int pollagain = TRUE;
  u_int32_t seq = 1, rg_err_count = 0;

  struct extra_primitives extras;
  struct primitives_ptrs prim_ptrs;
  char *dataptr;

  memcpy(&config, cfgptr, sizeof(struct configuration));
  memcpy(&extras, &((struct channels_list_entry *)ptr)->extras, sizeof(struct extra_primitives));
  recollect_pipe_memory(ptr);
  pm_setproctitle("%s [%s]", "Print Plugin", config.name);

  P_set_signals();
  P_init_default_values();
  pipebuf = (unsigned char *) Malloc(config.buffer_size);
  memset(pipebuf, 0, config.buffer_size);

  is_event = FALSE;
  if (!config.print_output)
    config.print_output = PRINT_OUTPUT_FORMATTED;
  else if (config.print_output & PRINT_OUTPUT_EVENT)
    is_event = TRUE;

  timeout = config.sql_refresh_time*1000;

  /* setting function pointers */
  if (config.what_to_count & (COUNT_SUM_HOST|COUNT_SUM_NET))
    insert_func = P_sum_host_insert;
  else if (config.what_to_count & COUNT_SUM_PORT) insert_func = P_sum_port_insert;
  else if (config.what_to_count & COUNT_SUM_AS) insert_func = P_sum_as_insert;
#if defined (HAVE_L2)
  else if (config.what_to_count & COUNT_SUM_MAC) insert_func = P_sum_mac_insert;
#endif
  else insert_func = P_cache_insert;
  purge_func = P_cache_purge;

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

  if (!config.print_output_separator) config.print_output_separator = default_separator;

  if (!config.sql_table && config.print_output & PRINT_OUTPUT_FORMATTED)
    P_write_stats_header_formatted(stdout, is_event);
  else if (!config.sql_table && config.print_output & PRINT_OUTPUT_CSV)
    P_write_stats_header_csv(stdout, is_event);

  if (config.sql_table && (strchr(config.sql_table, '%') || strchr(config.sql_table, '$')))
    dyn_table = TRUE;
  else
    dyn_table = FALSE;

  /* plugin main loop */
  for(;;) {
    poll_again:
    status->wakeup = TRUE;
    calc_refresh_timeout(refresh_deadline, now, &timeout);
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

void P_cache_purge(struct chained_cache *queue[], int index)
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
  char src_mac[18], dst_mac[18], src_host[INET6_ADDRSTRLEN], dst_host[INET6_ADDRSTRLEN], ip_address[INET6_ADDRSTRLEN];
  char rd_str[SRVBUFLEN], *sep = config.print_output_separator;
  char *as_path, *bgp_comm, empty_string[] = "", empty_aspath[] = "^$", empty_ip4[] = "0.0.0.0", empty_ip6[] = "::";
  char empty_macaddress[] = "00:00:00:00:00:00", empty_rd[] = "0:0";
  FILE *f = NULL;
  int j, is_event = FALSE, qn = 0, go_to_pending;
  time_t start, duration;
  char tmpbuf[LONGLONGSRVBUFLEN], current_table[SRVBUFLEN], elem_table[SRVBUFLEN];
  struct primitives_ptrs prim_ptrs, elem_prim_ptrs;
  struct pkt_data dummy_data, elem_dummy_data;
  pid_t writer_pid = getpid();

  if (!index) return;

  empty_pcust = malloc(config.cpptrs.len);
  if (!empty_pcust) {
    Log(LOG_ERR, "ERROR ( %s/%s ): Unable to malloc() empty_pcust. Exiting.\n", config.name, config.type);
    exit_plugin(1);
  }

  memset(&empty_pbgp, 0, sizeof(struct pkt_bgp_primitives));
  memset(&empty_pnat, 0, sizeof(struct pkt_nat_primitives));
  memset(&empty_pmpls, 0, sizeof(struct pkt_mpls_primitives));
  memset(empty_pcust, 0, config.cpptrs.len);
  memset(&prim_ptrs, 0, sizeof(prim_ptrs));
  memset(&dummy_data, 0, sizeof(dummy_data));
  memset(&elem_prim_ptrs, 0, sizeof(elem_prim_ptrs));
  memset(&elem_dummy_data, 0, sizeof(elem_dummy_data));

  memcpy(pending_queries_queue, queue, index*sizeof(struct db_cache *));
  pqq_ptr = index;

  Log(LOG_INFO, "INFO ( %s/%s ): *** Purging cache - START (PID: %u) ***\n", config.name, config.type, writer_pid);
  start = time(NULL);

  start:
  memcpy(queue, pending_queries_queue, pqq_ptr*sizeof(struct db_cache *));
  memset(pending_queries_queue, 0, pqq_ptr*sizeof(struct db_cache *));
  index = pqq_ptr; pqq_ptr = 0;

  if (config.print_output & PRINT_OUTPUT_EVENT) is_event = TRUE;

  if (config.sql_table) {
    time_t stamp = 0;
    int append = FALSE;

    strlcpy(current_table, config.sql_table, SRVBUFLEN);

    if (dyn_table) {
      stamp = queue[0]->basetime.tv_sec;

      prim_ptrs.data = &dummy_data;
      primptrs_set_all_from_chained_cache(&prim_ptrs, queue[0]);

      handle_dynname_internal_strings_same(tmpbuf, LONGSRVBUFLEN, current_table, &prim_ptrs);
      strftime_same(current_table, LONGSRVBUFLEN, tmpbuf, &stamp);
    }

    f = open_print_output_file(current_table, &append);

    if (f && !append) { 
      if (config.print_markers) fprintf(f, "--START (%ld+%d)--\n", stamp, config.sql_refresh_time);

      if (config.print_output & PRINT_OUTPUT_FORMATTED)
        P_write_stats_header_formatted(f, is_event);
      else if (config.print_output & PRINT_OUTPUT_CSV)
        P_write_stats_header_csv(f, is_event);
    }
  }
  else f = stdout; /* write to standard output */

  for (j = 0; j < index; j++) {
    int count = 0;
    go_to_pending = FALSE;

    if (dyn_table) {
      time_t stamp = 0;

      memset(tmpbuf, 0, LONGLONGSRVBUFLEN); // XXX: pedantic?
      stamp = queue[j]->basetime.tv_sec;
      strlcpy(elem_table, config.sql_table, SRVBUFLEN);

      elem_prim_ptrs.data = &elem_dummy_data;
      primptrs_set_all_from_chained_cache(&elem_prim_ptrs, queue[j]);
      handle_dynname_internal_strings_same(tmpbuf, LONGSRVBUFLEN, elem_table, &elem_prim_ptrs);
      strftime_same(elem_table, LONGSRVBUFLEN, tmpbuf, &stamp);

      if (strncmp(current_table, elem_table, SRVBUFLEN)) {
        pending_queries_queue[pqq_ptr] = queue[j];

        pqq_ptr++;
        go_to_pending = TRUE;
      }
    }

    if (!go_to_pending) {
      qn++;

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
  
      if (f && config.print_output & PRINT_OUTPUT_FORMATTED) {
        if (config.what_to_count & COUNT_TAG) fprintf(f, "%-10llu  ", data->tag);
        if (config.what_to_count & COUNT_TAG2) fprintf(f, "%-10llu  ", data->tag2);
        if (config.what_to_count & COUNT_CLASS) fprintf(f, "%-16s  ", ((data->class && class[(data->class)-1].id) ? class[(data->class)-1].protocol : "unknown" ));
  #if defined (HAVE_L2)
        if (config.what_to_count & COUNT_SRC_MAC) {
          etheraddr_string(data->eth_shost, src_mac);
  	if (strlen(src_mac))
            fprintf(f, "%-17s  ", src_mac);
          else
            fprintf(f, "%-17s  ", empty_macaddress);
        }
        if (config.what_to_count & COUNT_DST_MAC) {
          etheraddr_string(data->eth_dhost, dst_mac);
  	if (strlen(dst_mac))
            fprintf(f, "%-17s  ", dst_mac);
  	else
            fprintf(f, "%-17s  ", empty_macaddress);
        }
        if (config.what_to_count & COUNT_VLAN) fprintf(f, "%-5u  ", data->vlan_id); 
        if (config.what_to_count & COUNT_COS) fprintf(f, "%-2u  ", data->cos); 
        if (config.what_to_count & COUNT_ETHERTYPE) fprintf(f, "%-5x  ", data->etype); 
  #endif
        if (config.what_to_count & COUNT_SRC_AS) fprintf(f, "%-10u  ", data->src_as); 
        if (config.what_to_count & COUNT_DST_AS) fprintf(f, "%-10u  ", data->dst_as); 
  
        if (config.what_to_count & COUNT_STD_COMM) { 
          bgp_comm = pbgp->std_comms;
          while (bgp_comm) {
            bgp_comm = strchr(pbgp->std_comms, ' ');
            if (bgp_comm) *bgp_comm = '_';
          }
  
          if (strlen(pbgp->std_comms)) 
            fprintf(f, "%-22s   ", pbgp->std_comms);
          else
  	  fprintf(f, "%-22u   ", 0);
        }

        if (config.what_to_count & COUNT_EXT_COMM && !(config.what_to_count & COUNT_STD_COMM)) {
          bgp_comm = pbgp->ext_comms;
          while (bgp_comm) {
            bgp_comm = strchr(pbgp->ext_comms, ' ');
            if (bgp_comm) *bgp_comm = '_';
          }

          if (strlen(pbgp->ext_comms))
            fprintf(f, "%-22s   ", pbgp->ext_comms);
          else
          fprintf(f, "%-22u   ", 0);
        }

        if (config.what_to_count & COUNT_SRC_STD_COMM) {
          bgp_comm = pbgp->src_std_comms;
          while (bgp_comm) {
            bgp_comm = strchr(pbgp->src_std_comms, ' ');
            if (bgp_comm) *bgp_comm = '_';
          }

          if (strlen(pbgp->src_std_comms))
            fprintf(f, "%-22s   ", pbgp->src_std_comms);
          else
          fprintf(f, "%-22u   ", 0);
        }

        if (config.what_to_count & COUNT_SRC_EXT_COMM && !(config.what_to_count & COUNT_SRC_STD_COMM)) {
          bgp_comm = pbgp->src_ext_comms;
          while (bgp_comm) {
            bgp_comm = strchr(pbgp->src_ext_comms, ' ');
            if (bgp_comm) *bgp_comm = '_';
          }

          if (strlen(pbgp->src_ext_comms))
            fprintf(f, "%-22s   ", pbgp->src_ext_comms);
          else
          fprintf(f, "%-22u   ", 0);
        }
  
        if (config.what_to_count & COUNT_AS_PATH) {
          as_path = pbgp->as_path;
          while (as_path) {
  	    as_path = strchr(pbgp->as_path, ' ');
  	    if (as_path) *as_path = '_';
          }
          if (strlen(pbgp->as_path))
  	  fprintf(f, "%-22s   ", pbgp->as_path);
          else
  	  fprintf(f, "%-22s   ", empty_aspath);
        }

        if (config.what_to_count & COUNT_SRC_AS_PATH) {
          as_path = pbgp->src_as_path;
          while (as_path) {
            as_path = strchr(pbgp->src_as_path, ' ');
            if (as_path) *as_path = '_';
          }
          if (strlen(pbgp->src_as_path))
          fprintf(f, "%-22s   ", pbgp->src_as_path);
          else
          fprintf(f, "%-22s   ", empty_aspath);
        }
  
        if (config.what_to_count & COUNT_LOCAL_PREF) fprintf(f, "%-7u  ", pbgp->local_pref);
        if (config.what_to_count & COUNT_SRC_LOCAL_PREF) fprintf(f, "%-7u  ", pbgp->src_local_pref);
        if (config.what_to_count & COUNT_MED) fprintf(f, "%-6u  ", pbgp->med);
        if (config.what_to_count & COUNT_SRC_MED) fprintf(f, "%-6u  ", pbgp->src_med);

        if (config.what_to_count & COUNT_PEER_SRC_AS) fprintf(f, "%-10u  ", pbgp->peer_src_as);
        if (config.what_to_count & COUNT_PEER_DST_AS) fprintf(f, "%-10u  ", pbgp->peer_dst_as);
  
        if (config.what_to_count & COUNT_PEER_SRC_IP) {
          addr_to_str(ip_address, &pbgp->peer_src_ip);
  #if defined ENABLE_IPV6
          if (strlen(ip_address))
            fprintf(f, "%-45s  ", ip_address);
  	else
            fprintf(f, "%-45s  ", empty_ip6);
  #else
  	if (strlen(ip_address))
            fprintf(f, "%-15s  ", ip_address);
  	else
            fprintf(f, "%-15s  ", empty_ip4);
  #endif
        }
        if (config.what_to_count & COUNT_PEER_DST_IP) {
          addr_to_str(ip_address, &pbgp->peer_dst_ip);
  #if defined ENABLE_IPV6
          if (strlen(ip_address))
            fprintf(f, "%-45s  ", ip_address);
          else
            fprintf(f, "%-45s  ", empty_ip6);
  #else
          if (strlen(ip_address))
            fprintf(f, "%-15s  ", ip_address);
          else 
            fprintf(f, "%-15s  ", empty_ip4);
  #endif
        }
  
        if (config.what_to_count & COUNT_IN_IFACE) fprintf(f, "%-10u  ", data->ifindex_in);
        if (config.what_to_count & COUNT_OUT_IFACE) fprintf(f, "%-10u  ", data->ifindex_out);
  
        if (config.what_to_count & COUNT_MPLS_VPN_RD) {
          bgp_rd2str(rd_str, &pbgp->mpls_vpn_rd);
  	if (strlen(rd_str))
            fprintf(f, "%-18s  ", rd_str);
  	else
            fprintf(f, "%-18s  ", empty_rd);
        }
  
        if (config.what_to_count & (COUNT_SRC_HOST|COUNT_SRC_NET)) {
          addr_to_str(src_host, &data->src_ip);
  #if defined ENABLE_IPV6
  	if (strlen(src_host))
            fprintf(f, "%-45s  ", src_host);
  	else
            fprintf(f, "%-45s  ", empty_ip6);
  #else
  	if (strlen(src_host))
            fprintf(f, "%-15s  ", src_host);
  	else
            fprintf(f, "%-15s  ", empty_ip4);
  #endif
        }
        if (config.what_to_count & (COUNT_DST_HOST|COUNT_DST_NET)) {
          addr_to_str(dst_host, &data->dst_ip);
  #if defined ENABLE_IPV6
  	if (strlen(dst_host))
            fprintf(f, "%-45s  ", dst_host);
  	else
            fprintf(f, "%-45s  ", empty_ip6);
  #else
  	if (strlen(dst_host))
            fprintf(f, "%-15s  ", dst_host);
  	else
            fprintf(f, "%-15s  ", empty_ip4);
  #endif
        }
        if (config.what_to_count & COUNT_SRC_NMASK) fprintf(f, "%-3u       ", data->src_nmask);
        if (config.what_to_count & COUNT_DST_NMASK) fprintf(f, "%-3u       ", data->dst_nmask);
        if (config.what_to_count & COUNT_SRC_PORT) fprintf(f, "%-5u     ", data->src_port);
        if (config.what_to_count & COUNT_DST_PORT) fprintf(f, "%-5u     ", data->dst_port);
        if (config.what_to_count & COUNT_TCPFLAGS) fprintf(f, "%-3u        ", queue[j]->tcp_flags);
  
        if (config.what_to_count & COUNT_IP_PROTO) {
          if (!config.num_protos) fprintf(f, "%-10s  ", _protocols[data->proto].name);
          else  fprintf(f, "%-10d  ", _protocols[data->proto].number);
        }
  
        if (config.what_to_count & COUNT_IP_TOS) fprintf(f, "%-3u    ", data->tos);
  
  #if defined WITH_GEOIP
        if (config.what_to_count_2 & COUNT_SRC_HOST_COUNTRY) fprintf(f, "%-5s       ", GeoIP_code_by_id(data->src_ip_country));
        if (config.what_to_count_2 & COUNT_DST_HOST_COUNTRY) fprintf(f, "%-5s       ", GeoIP_code_by_id(data->dst_ip_country));
  #endif
  
        if (config.what_to_count_2 & COUNT_SAMPLING_RATE) fprintf(f, "%-7u       ", data->sampling_rate);
        if (config.what_to_count_2 & COUNT_PKT_LEN_DISTRIB) fprintf(f, "%-10s      ", config.pkt_len_distrib_bins[data->pkt_len_distrib]);
  
        if (config.what_to_count_2 & COUNT_POST_NAT_SRC_HOST) {
          addr_to_str(ip_address, &pnat->post_nat_src_ip);
  
  #if defined ENABLE_IPV6
          if (strlen(ip_address))
            fprintf(f, "%-45s  ", ip_address);
          else
            fprintf(f, "%-45s  ", empty_ip6);
  #else
          if (strlen(ip_address))
            fprintf(f, "%-15s  ", ip_address);
          else
            fprintf(f, "%-15s  ", empty_ip4);
  #endif
        }
  
        if (config.what_to_count_2 & COUNT_POST_NAT_DST_HOST) {
          addr_to_str(ip_address, &pnat->post_nat_dst_ip);
  
  #if defined ENABLE_IPV6
          if (strlen(ip_address))
            fprintf(f, "%-45s  ", ip_address);
          else
            fprintf(f, "%-45s  ", empty_ip6);
  #else 
          if (strlen(ip_address))
            fprintf(f, "%-15s  ", ip_address);
          else
            fprintf(f, "%-15s  ", empty_ip4);
  #endif
        }
  
        if (config.what_to_count_2 & COUNT_POST_NAT_SRC_PORT) fprintf(f, "%-5u              ", pnat->post_nat_src_port);
        if (config.what_to_count_2 & COUNT_POST_NAT_DST_PORT) fprintf(f, "%-5u              ", pnat->post_nat_dst_port);
        if (config.what_to_count_2 & COUNT_NAT_EVENT) fprintf(f, "%-3u       ", pnat->nat_event);
  
        if (config.what_to_count_2 & COUNT_MPLS_LABEL_TOP) {
  	fprintf(f, "%-7u         ", pmpls->mpls_label_top);
        }
        if (config.what_to_count_2 & COUNT_MPLS_LABEL_BOTTOM) {
  	fprintf(f, "%-7u            ", pmpls->mpls_label_bottom);
        }
        if (config.what_to_count_2 & COUNT_MPLS_STACK_DEPTH) {
  	fprintf(f, "%-2u                ", pmpls->mpls_stack_depth);
        }
  
        if (config.what_to_count_2 & COUNT_TIMESTAMP_START) {
          char buf1[SRVBUFLEN], buf2[SRVBUFLEN];
          time_t time1;
          struct tm *time2;
  
          time1 = pnat->timestamp_start.tv_sec;
          time2 = localtime(&time1);
          strftime(buf1, SRVBUFLEN, "%Y-%m-%d %H:%M:%S", time2);
          snprintf(buf2, SRVBUFLEN, "%s.%u", buf1, pnat->timestamp_start.tv_usec);
          fprintf(f, "%-30s ", buf2);
        }
  
        if (config.what_to_count_2 & COUNT_TIMESTAMP_END) {
          char buf1[SRVBUFLEN], buf2[SRVBUFLEN];
          time_t time1;
          struct tm *time2;
        
          time1 = pnat->timestamp_end.tv_sec;
          time2 = localtime(&time1);
          strftime(buf1, SRVBUFLEN, "%Y-%m-%d %H:%M:%S", time2);
          snprintf(buf2, SRVBUFLEN, "%s.%u", buf1, pnat->timestamp_end.tv_usec);
          fprintf(f, "%-30s ", buf2);
        }
  
        /* all custom primitives printed here */
        {
          char cp_str[SRVBUFLEN];
          int cp_idx;
  
          for (cp_idx = 0; cp_idx < config.cpptrs.num; cp_idx++) {
            custom_primitive_value_print(cp_str, SRVBUFLEN, pcust, &config.cpptrs.primitive[cp_idx], TRUE);
  	  fprintf(f, "%s  ", cp_str);
          }
        }
  
        if (!is_event) {
  #if defined HAVE_64BIT_COUNTERS
          fprintf(f, "%-20llu  ", queue[j]->packet_counter);
          if (config.what_to_count & COUNT_FLOWS) fprintf(f, "%-20llu  ", queue[j]->flow_counter);
          fprintf(f, "%llu\n", queue[j]->bytes_counter);
  #else
          fprintf(f, "%-10lu  ", queue[j]->packet_counter);
          if (config.what_to_count & COUNT_FLOWS) fprintf(f, "%-10lu  ", queue[j]->flow_counter);
          fprintf(f, "%lu\n", queue[j]->bytes_counter);
  #endif
        }
        else fprintf(f, "\n");
      }
      else if (f && config.print_output & PRINT_OUTPUT_CSV) {
        if (config.what_to_count & COUNT_TAG) fprintf(f, "%s%llu", write_sep(sep, &count), data->tag);
        if (config.what_to_count & COUNT_TAG2) fprintf(f, "%s%llu", write_sep(sep, &count), data->tag2);
        if (config.what_to_count & COUNT_CLASS) fprintf(f, "%s%s", write_sep(sep, &count), ((data->class && class[(data->class)-1].id) ? class[(data->class)-1].protocol : "unknown" ));
  #if defined (HAVE_L2)
        if (config.what_to_count & COUNT_SRC_MAC) {
          etheraddr_string(data->eth_shost, src_mac);
          fprintf(f, "%s%s", write_sep(sep, &count), src_mac);
        }
        if (config.what_to_count & COUNT_DST_MAC) {
          etheraddr_string(data->eth_dhost, dst_mac);
          fprintf(f, "%s%s", write_sep(sep, &count), dst_mac);
        }
        if (config.what_to_count & COUNT_VLAN) fprintf(f, "%s%u", write_sep(sep, &count), data->vlan_id); 
        if (config.what_to_count & COUNT_COS) fprintf(f, "%s%u", write_sep(sep, &count), data->cos); 
        if (config.what_to_count & COUNT_ETHERTYPE) fprintf(f, "%s%x", write_sep(sep, &count), data->etype); 
  #endif
        if (config.what_to_count & COUNT_SRC_AS) fprintf(f, "%s%u", write_sep(sep, &count), data->src_as); 
        if (config.what_to_count & COUNT_DST_AS) fprintf(f, "%s%u", write_sep(sep, &count), data->dst_as); 
  
        if (config.what_to_count & COUNT_STD_COMM) {
          bgp_comm = pbgp->std_comms;
          while (bgp_comm) {
            bgp_comm = strchr(pbgp->std_comms, ' ');
            if (bgp_comm) *bgp_comm = '_';
          }
  
          if (strlen(pbgp->std_comms)) 
            fprintf(f, "%s%s", write_sep(sep, &count), pbgp->std_comms);
          else
            fprintf(f, "%s%s", write_sep(sep, &count), empty_string);
        }

        if (config.what_to_count & COUNT_EXT_COMM && !(config.what_to_count & COUNT_STD_COMM)) {
          bgp_comm = pbgp->ext_comms;
          while (bgp_comm) {
            bgp_comm = strchr(pbgp->ext_comms, ' ');
            if (bgp_comm) *bgp_comm = '_';
          }

          if (strlen(pbgp->ext_comms))
            fprintf(f, "%s%s", write_sep(sep, &count), pbgp->ext_comms);
          else
            fprintf(f, "%s%s", write_sep(sep, &count), empty_string);
        }

        if (config.what_to_count & COUNT_SRC_STD_COMM) {
          bgp_comm = pbgp->src_std_comms;
          while (bgp_comm) {
            bgp_comm = strchr(pbgp->src_std_comms, ' ');
            if (bgp_comm) *bgp_comm = '_';
          }

          if (strlen(pbgp->src_std_comms))
            fprintf(f, "%s%s", write_sep(sep, &count), pbgp->src_std_comms);
          else
            fprintf(f, "%s%s", write_sep(sep, &count), empty_string);
        }

        if (config.what_to_count & COUNT_SRC_EXT_COMM && !(config.what_to_count & COUNT_SRC_STD_COMM)) {
          bgp_comm = pbgp->src_ext_comms;
          while (bgp_comm) {
            bgp_comm = strchr(pbgp->src_ext_comms, ' ');
            if (bgp_comm) *bgp_comm = '_';
          }

          if (strlen(pbgp->src_ext_comms))
            fprintf(f, "%s%s", write_sep(sep, &count), pbgp->src_ext_comms);
          else
            fprintf(f, "%s%s", write_sep(sep, &count), empty_string);
        }
  
        if (config.what_to_count & COUNT_AS_PATH) {
          as_path = pbgp->as_path;
          while (as_path) {
  	    as_path = strchr(pbgp->as_path, ' ');
  	    if (as_path) *as_path = '_';
          }

	  if (strlen(pbgp->as_path))
            fprintf(f, "%s%s", write_sep(sep, &count), pbgp->as_path);
	  else
	    fprintf(f, "%s%s", write_sep(sep, &count), empty_string);
        }

        if (config.what_to_count & COUNT_SRC_AS_PATH) {
          as_path = pbgp->src_as_path;
          while (as_path) {
            as_path = strchr(pbgp->src_as_path, ' ');
            if (as_path) *as_path = '_';
          }

          if (strlen(pbgp->src_as_path))
            fprintf(f, "%s%s", write_sep(sep, &count), pbgp->src_as_path);
          else
            fprintf(f, "%s%s", write_sep(sep, &count), empty_string);
        }
  
        if (config.what_to_count & COUNT_LOCAL_PREF) fprintf(f, "%s%u", write_sep(sep, &count), pbgp->local_pref);
        if (config.what_to_count & COUNT_SRC_LOCAL_PREF) fprintf(f, "%s%u", write_sep(sep, &count), pbgp->src_local_pref);
        if (config.what_to_count & COUNT_MED) fprintf(f, "%s%u", write_sep(sep, &count), pbgp->med);
        if (config.what_to_count & COUNT_SRC_MED) fprintf(f, "%s%u", write_sep(sep, &count), pbgp->src_med);

        if (config.what_to_count & COUNT_PEER_SRC_AS) fprintf(f, "%s%u", write_sep(sep, &count), pbgp->peer_src_as);
        if (config.what_to_count & COUNT_PEER_DST_AS) fprintf(f, "%s%u", write_sep(sep, &count), pbgp->peer_dst_as);
  
        if (config.what_to_count & COUNT_PEER_SRC_IP) {
          addr_to_str(ip_address, &pbgp->peer_src_ip);
          fprintf(f, "%s%s", write_sep(sep, &count), ip_address);
        }
        if (config.what_to_count & COUNT_PEER_DST_IP) {
          addr_to_str(ip_address, &pbgp->peer_dst_ip);
          fprintf(f, "%s%s", write_sep(sep, &count), ip_address);
        }
  
        if (config.what_to_count & COUNT_IN_IFACE) fprintf(f, "%s%u", write_sep(sep, &count), data->ifindex_in);
        if (config.what_to_count & COUNT_OUT_IFACE) fprintf(f, "%s%u", write_sep(sep, &count), data->ifindex_out);
  
        if (config.what_to_count & COUNT_MPLS_VPN_RD) {
          bgp_rd2str(rd_str, &pbgp->mpls_vpn_rd);
          fprintf(f, "%s%s", write_sep(sep, &count), rd_str);
        }
  
        if (config.what_to_count & (COUNT_SRC_HOST|COUNT_SRC_NET)) {
          addr_to_str(src_host, &data->src_ip);
          fprintf(f, "%s%s", write_sep(sep, &count), src_host);
        }
        if (config.what_to_count & (COUNT_DST_HOST|COUNT_DST_NET)) {
          addr_to_str(dst_host, &data->dst_ip);
          fprintf(f, "%s%s", write_sep(sep, &count), dst_host);
        }
  
        if (config.what_to_count & COUNT_SRC_NMASK) fprintf(f, "%s%u", write_sep(sep, &count), data->src_nmask);
        if (config.what_to_count & COUNT_DST_NMASK) fprintf(f, "%s%u", write_sep(sep, &count), data->dst_nmask);
        if (config.what_to_count & COUNT_SRC_PORT) fprintf(f, "%s%u", write_sep(sep, &count), data->src_port);
        if (config.what_to_count & COUNT_DST_PORT) fprintf(f, "%s%u", write_sep(sep, &count), data->dst_port);
        if (config.what_to_count & COUNT_TCPFLAGS) fprintf(f, "%s%u", write_sep(sep, &count), queue[j]->tcp_flags);
  
        if (config.what_to_count & COUNT_IP_PROTO) {
          if (!config.num_protos) fprintf(f, "%s%s", write_sep(sep, &count), _protocols[data->proto].name);
          else fprintf(f, "%s%d", write_sep(sep, &count), _protocols[data->proto].number);
        }
  
        if (config.what_to_count & COUNT_IP_TOS) fprintf(f, "%s%u", write_sep(sep, &count), data->tos);
  
  #if defined WITH_GEOIP
        if (config.what_to_count_2 & COUNT_SRC_HOST_COUNTRY) fprintf(f, "%s%s", write_sep(sep, &count), GeoIP_code_by_id(data->src_ip_country));
        if (config.what_to_count_2 & COUNT_DST_HOST_COUNTRY) fprintf(f, "%s%s", write_sep(sep, &count), GeoIP_code_by_id(data->dst_ip_country));
  #endif
  
        if (config.what_to_count_2 & COUNT_SAMPLING_RATE) fprintf(f, "%s%u", write_sep(sep, &count), data->sampling_rate);
        if (config.what_to_count_2 & COUNT_PKT_LEN_DISTRIB) fprintf(f, "%s%s", write_sep(sep, &count), config.pkt_len_distrib_bins[data->pkt_len_distrib]);
  
        if (config.what_to_count_2 & COUNT_POST_NAT_SRC_HOST) {
          addr_to_str(src_host, &pnat->post_nat_src_ip);
          fprintf(f, "%s%s", write_sep(sep, &count), src_host);
        }
        if (config.what_to_count_2 & COUNT_POST_NAT_DST_HOST) {
          addr_to_str(dst_host, &pnat->post_nat_dst_ip);
          fprintf(f, "%s%s", write_sep(sep, &count), dst_host);
        }
        if (config.what_to_count_2 & COUNT_POST_NAT_SRC_PORT) fprintf(f, "%s%u", write_sep(sep, &count), pnat->post_nat_src_port);
        if (config.what_to_count_2 & COUNT_POST_NAT_DST_PORT) fprintf(f, "%s%u", write_sep(sep, &count), pnat->post_nat_dst_port);
        if (config.what_to_count_2 & COUNT_NAT_EVENT) fprintf(f, "%s%u", write_sep(sep, &count), pnat->nat_event);
  
        if (config.what_to_count_2 & COUNT_MPLS_LABEL_TOP) fprintf(f, "%s%u", write_sep(sep, &count), pmpls->mpls_label_top);
        if (config.what_to_count_2 & COUNT_MPLS_LABEL_BOTTOM) fprintf(f, "%s%u", write_sep(sep, &count), pmpls->mpls_label_bottom);
        if (config.what_to_count_2 & COUNT_MPLS_STACK_DEPTH) fprintf(f, "%s%u", write_sep(sep, &count), pmpls->mpls_stack_depth);
  
        if (config.what_to_count_2 & COUNT_TIMESTAMP_START) {
            char buf1[SRVBUFLEN], buf2[SRVBUFLEN];
            time_t time1;
            struct tm *time2;
  
            time1 = pnat->timestamp_start.tv_sec;
            time2 = localtime(&time1);
            strftime(buf1, SRVBUFLEN, "%Y-%m-%d %H:%M:%S", time2);
            snprintf(buf2, SRVBUFLEN, "%s.%u", buf1, pnat->timestamp_start.tv_usec);
            fprintf(f, "%s%s", write_sep(sep, &count), buf2);
        }
  
        if (config.what_to_count_2 & COUNT_TIMESTAMP_END) {
            char buf1[SRVBUFLEN], buf2[SRVBUFLEN];
            time_t time1;
            struct tm *time2;
  
            time1 = pnat->timestamp_end.tv_sec;
            time2 = localtime(&time1);
            strftime(buf1, SRVBUFLEN, "%Y-%m-%d %H:%M:%S", time2);
            snprintf(buf2, SRVBUFLEN, "%s.%u", buf1, pnat->timestamp_end.tv_usec);
            fprintf(f, "%s%s", write_sep(sep, &count), buf2);
        }
  
        /* all custom primitives printed here */
        {
          char cp_str[SRVBUFLEN];
          int cp_idx;
  
          for (cp_idx = 0; cp_idx < config.cpptrs.num; cp_idx++) {
            custom_primitive_value_print(cp_str, SRVBUFLEN, pcust, &config.cpptrs.primitive[cp_idx], FALSE);
            fprintf(f, "%s%s", write_sep(sep, &count), cp_str);
          }
        }
  
        if (!is_event) {
  #if defined HAVE_64BIT_COUNTERS
          fprintf(f, "%s%llu", write_sep(sep, &count), queue[j]->packet_counter);
          if (config.what_to_count & COUNT_FLOWS) fprintf(f, "%s%llu", write_sep(sep, &count), queue[j]->flow_counter);
          fprintf(f, "%s%llu\n", write_sep(sep, &count), queue[j]->bytes_counter);
  #else
          fprintf(f, "%s%lu", write_sep(sep, &count), queue[j]->packet_counter);
          if (config.what_to_count & COUNT_FLOWS) fprintf(f, "%s%lu", write_sep(sep, &count), queue[j]->flow_counter);
          fprintf(f, "%s%lu\n", write_sep(sep, &count), queue[j]->bytes_counter);
  #endif
        }
        else fprintf(f, "\n");
      }
      else if (f && config.print_output & PRINT_OUTPUT_JSON) {
        char *json_str;
  
        json_str = compose_json(config.what_to_count, config.what_to_count_2, queue[j]->flow_type,
                           &queue[j]->primitives, pbgp, pnat, pmpls, pcust, queue[j]->bytes_counter,
  			 queue[j]->packet_counter, queue[j]->flow_counter, queue[j]->tcp_flags, NULL);
  
        if (json_str) {
          fprintf(f, "%s\n", json_str);
          free(json_str);
        }
      }
    }
  }

  if (f && config.print_markers) fprintf(f, "--END--\n");

  if (f && config.sql_table) close_print_output_file(f, config.print_latest_file, current_table, &prim_ptrs);

  /* If we have pending queries then start again */
  if (pqq_ptr) goto start;

  duration = time(NULL)-start;
  Log(LOG_INFO, "INFO ( %s/%s ): *** Purging cache - END (PID: %u, QN: %u, ET: %u) ***\n",
		config.name, config.type, writer_pid, qn, duration);

  if (config.sql_trigger_exec) P_trigger_exec(config.sql_trigger_exec); 
}

void P_write_stats_header_formatted(FILE *f, int is_event)
{
  if (config.what_to_count & COUNT_TAG) fprintf(f, "TAG         ");
  if (config.what_to_count & COUNT_TAG2) fprintf(f, "TAG2        ");
  if (config.what_to_count & COUNT_CLASS) fprintf(f, "CLASS             ");
#if defined HAVE_L2
  if (config.what_to_count & COUNT_SRC_MAC) fprintf(f, "SRC_MAC            ");
  if (config.what_to_count & COUNT_DST_MAC) fprintf(f, "DST_MAC            ");
  if (config.what_to_count & COUNT_VLAN) fprintf(f, "VLAN   ");
  if (config.what_to_count & COUNT_COS) fprintf(f, "COS ");
  if (config.what_to_count & COUNT_ETHERTYPE) fprintf(f, "ETYPE  ");
#endif
  if (config.what_to_count & COUNT_SRC_AS) fprintf(f, "SRC_AS      ");
  if (config.what_to_count & COUNT_DST_AS) fprintf(f, "DST_AS      ");
  if (config.what_to_count & (COUNT_STD_COMM|COUNT_EXT_COMM)) fprintf(f, "COMMS                    ");
  if (config.what_to_count & (COUNT_SRC_STD_COMM|COUNT_SRC_EXT_COMM)) fprintf(f, "SRC_COMMS                ");
  if (config.what_to_count & COUNT_AS_PATH) fprintf(f, "AS_PATH                  ");
  if (config.what_to_count & COUNT_SRC_AS_PATH) fprintf(f, "SRC_AS_PATH              ");
  if (config.what_to_count & COUNT_LOCAL_PREF) fprintf(f, "PREF     ");
  if (config.what_to_count & COUNT_SRC_LOCAL_PREF) fprintf(f, "SRC_PREF ");
  if (config.what_to_count & COUNT_MED) fprintf(f, "MED     ");
  if (config.what_to_count & COUNT_SRC_MED) fprintf(f, "SRC_MED ");
  if (config.what_to_count & COUNT_PEER_SRC_AS) fprintf(f, "PEER_SRC_AS ");
  if (config.what_to_count & COUNT_PEER_DST_AS) fprintf(f, "PEER_DST_AS ");
  if (config.what_to_count & COUNT_PEER_SRC_IP) fprintf(f, "PEER_SRC_IP      ");
  if (config.what_to_count & COUNT_PEER_DST_IP) fprintf(f, "PEER_DST_IP      ");
  if (config.what_to_count & COUNT_IN_IFACE) fprintf(f, "IN_IFACE    ");
  if (config.what_to_count & COUNT_OUT_IFACE) fprintf(f, "OUT_IFACE   ");
  if (config.what_to_count & COUNT_MPLS_VPN_RD) fprintf(f, "MPLS_VPN_RD         ");
#if defined ENABLE_IPV6
  if (config.what_to_count & (COUNT_SRC_HOST|COUNT_SRC_NET)) fprintf(f, "SRC_IP                                         ");
  if (config.what_to_count & (COUNT_DST_HOST|COUNT_DST_NET)) fprintf(f, "DST_IP                                         ");
#else
  if (config.what_to_count & (COUNT_SRC_HOST|COUNT_SRC_NET)) fprintf(f, "SRC_IP           ");
  if (config.what_to_count & (COUNT_DST_HOST|COUNT_DST_NET)) fprintf(f, "DST_IP           ");
#endif
  if (config.what_to_count & COUNT_SRC_NMASK) fprintf(f, "SRC_MASK  ");
  if (config.what_to_count & COUNT_DST_NMASK) fprintf(f, "DST_MASK  ");
  if (config.what_to_count & COUNT_SRC_PORT) fprintf(f, "SRC_PORT  ");
  if (config.what_to_count & COUNT_DST_PORT) fprintf(f, "DST_PORT  ");
  if (config.what_to_count & COUNT_TCPFLAGS) fprintf(f, "TCP_FLAGS  ");
  if (config.what_to_count & COUNT_IP_PROTO) fprintf(f, "PROTOCOL    ");
  if (config.what_to_count & COUNT_IP_TOS) fprintf(f, "TOS    ");
#if defined WITH_GEOIP
  if (config.what_to_count_2 & COUNT_SRC_HOST_COUNTRY) fprintf(f, "SH_COUNTRY  ");
  if (config.what_to_count_2 & COUNT_DST_HOST_COUNTRY) fprintf(f, "DH_COUNTRY  ");
#endif
  if (config.what_to_count_2 & COUNT_SAMPLING_RATE) fprintf(f, "SAMPLING_RATE ");
  if (config.what_to_count_2 & COUNT_PKT_LEN_DISTRIB) fprintf(f, "PKT_LEN_DISTRIB ");
#if defined ENABLE_IPV6
  if (config.what_to_count_2 & COUNT_POST_NAT_SRC_HOST) fprintf(f, "POST_NAT_SRC_IP                                ");
  if (config.what_to_count_2 & COUNT_POST_NAT_DST_HOST) fprintf(f, "POST_NAT_DST_IP                                ");
#else
  if (config.what_to_count_2 & COUNT_POST_NAT_SRC_HOST) fprintf(f, "POST_NAT_SRC_IP  ");
  if (config.what_to_count_2 & COUNT_POST_NAT_DST_HOST) fprintf(f, "POST_NAT_DST_IP  ");
#endif
  if (config.what_to_count_2 & COUNT_POST_NAT_SRC_PORT) fprintf(f, "POST_NAT_SRC_PORT  ");
  if (config.what_to_count_2 & COUNT_POST_NAT_DST_PORT) fprintf(f, "POST_NAT_DST_PORT  ");
  if (config.what_to_count_2 & COUNT_NAT_EVENT) fprintf(f, "NAT_EVENT ");
  if (config.what_to_count_2 & COUNT_MPLS_LABEL_TOP) fprintf(f, "MPLS_LABEL_TOP  ");
  if (config.what_to_count_2 & COUNT_MPLS_LABEL_BOTTOM) fprintf(f, "MPLS_LABEL_BOTTOM  ");
  if (config.what_to_count_2 & COUNT_MPLS_STACK_DEPTH) fprintf(f, "MPLS_STACK_DEPTH  ");
  if (config.what_to_count_2 & COUNT_TIMESTAMP_START) fprintf(f, "TIMESTAMP_START                ");
  if (config.what_to_count_2 & COUNT_TIMESTAMP_END) fprintf(f, "TIMESTAMP_END                  "); 

  /* all custom primitives printed here */
  {
    char cp_str[SRVBUFLEN];
    int cp_idx;

    for (cp_idx = 0; cp_idx < config.cpptrs.num; cp_idx++) {
      custom_primitive_header_print(cp_str, SRVBUFLEN, &config.cpptrs.primitive[cp_idx], TRUE);
      fprintf(f, "%s  ", cp_str);
    }
  }

  if (!is_event) {
#if defined HAVE_64BIT_COUNTERS
    fprintf(f, "PACKETS               ");
    if (config.what_to_count & COUNT_FLOWS) fprintf(f, "FLOWS                 ");
    fprintf(f, "BYTES\n");
#else
    fprintf(f, "PACKETS     ");
    if (config.what_to_count & COUNT_FLOWS) fprintf(f, "FLOWS       ");
    fprintf(f, "BYTES\n");
#endif
  }
  else fprintf(f, "\n");
}

void P_write_stats_header_csv(FILE *f, int is_event)
{
  char *sep = config.print_output_separator;
  int count = 0;

  if (config.what_to_count & COUNT_TAG) fprintf(f, "%sTAG", write_sep(sep, &count));
  if (config.what_to_count & COUNT_TAG2) fprintf(f, "%sTAG2", write_sep(sep, &count));
  if (config.what_to_count & COUNT_CLASS) fprintf(f, "%sCLASS", write_sep(sep, &count));
#if defined HAVE_L2
  if (config.what_to_count & COUNT_SRC_MAC) fprintf(f, "%sSRC_MAC", write_sep(sep, &count));
  if (config.what_to_count & COUNT_DST_MAC) fprintf(f, "%sDST_MAC", write_sep(sep, &count));
  if (config.what_to_count & COUNT_VLAN) fprintf(f, "%sVLAN", write_sep(sep, &count));
  if (config.what_to_count & COUNT_COS) fprintf(f, "%sCOS", write_sep(sep, &count));
  if (config.what_to_count & COUNT_ETHERTYPE) fprintf(f, "%sETYPE", write_sep(sep, &count));
#endif
  if (config.what_to_count & COUNT_SRC_AS) fprintf(f, "%sSRC_AS", write_sep(sep, &count));
  if (config.what_to_count & COUNT_DST_AS) fprintf(f, "%sDST_AS", write_sep(sep, &count));
  if (config.what_to_count & (COUNT_STD_COMM|COUNT_EXT_COMM)) fprintf(f, "%sCOMMS", write_sep(sep, &count));
  if (config.what_to_count & (COUNT_SRC_STD_COMM|COUNT_SRC_EXT_COMM)) fprintf(f, "%sSRC_COMMS", write_sep(sep, &count));
  if (config.what_to_count & COUNT_AS_PATH) fprintf(f, "%sAS_PATH", write_sep(sep, &count));
  if (config.what_to_count & COUNT_SRC_AS_PATH) fprintf(f, "%sSRC_AS_PATH", write_sep(sep, &count));
  if (config.what_to_count & COUNT_LOCAL_PREF) fprintf(f, "%sPREF", write_sep(sep, &count));
  if (config.what_to_count & COUNT_SRC_LOCAL_PREF) fprintf(f, "%sSRC_PREF", write_sep(sep, &count));
  if (config.what_to_count & COUNT_MED) fprintf(f, "%sMED", write_sep(sep, &count));
  if (config.what_to_count & COUNT_SRC_MED) fprintf(f, "%sSRC_MED", write_sep(sep, &count));
  if (config.what_to_count & COUNT_PEER_SRC_AS) fprintf(f, "%sPEER_SRC_AS", write_sep(sep, &count));
  if (config.what_to_count & COUNT_PEER_DST_AS) fprintf(f, "%sPEER_DST_AS", write_sep(sep, &count));
  if (config.what_to_count & COUNT_PEER_SRC_IP) fprintf(f, "%sPEER_SRC_IP", write_sep(sep, &count));
  if (config.what_to_count & COUNT_PEER_DST_IP) fprintf(f, "%sPEER_DST_IP", write_sep(sep, &count));
  if (config.what_to_count & COUNT_IN_IFACE) fprintf(f, "%sIN_IFACE", write_sep(sep, &count));
  if (config.what_to_count & COUNT_OUT_IFACE) fprintf(f, "%sOUT_IFACE", write_sep(sep, &count));
  if (config.what_to_count & COUNT_MPLS_VPN_RD) fprintf(f, "%sMPLS_VPN_RD", write_sep(sep, &count));
  if (config.what_to_count & (COUNT_SRC_HOST|COUNT_SRC_NET)) fprintf(f, "%sSRC_IP", write_sep(sep, &count));
  if (config.what_to_count & (COUNT_DST_HOST|COUNT_DST_NET)) fprintf(f, "%sDST_IP", write_sep(sep, &count));
  if (config.what_to_count & COUNT_SRC_NMASK) fprintf(f, "%sSRC_MASK", write_sep(sep, &count));
  if (config.what_to_count & COUNT_DST_NMASK) fprintf(f, "%sDST_MASK", write_sep(sep, &count));
  if (config.what_to_count & COUNT_SRC_PORT) fprintf(f, "%sSRC_PORT", write_sep(sep, &count));
  if (config.what_to_count & COUNT_DST_PORT) fprintf(f, "%sDST_PORT", write_sep(sep, &count));
  if (config.what_to_count & COUNT_TCPFLAGS) fprintf(f, "%sTCP_FLAGS", write_sep(sep, &count));
  if (config.what_to_count & COUNT_IP_PROTO) fprintf(f, "%sPROTOCOL", write_sep(sep, &count));
  if (config.what_to_count & COUNT_IP_TOS) fprintf(f, "%sTOS", write_sep(sep, &count));
#if defined WITH_GEOIP
  if (config.what_to_count_2 & COUNT_SRC_HOST_COUNTRY) fprintf(f, "%sSH_COUNTRY", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_DST_HOST_COUNTRY) fprintf(f, "%sDH_COUNTRY", write_sep(sep, &count));
#endif
  if (config.what_to_count_2 & COUNT_SAMPLING_RATE) fprintf(f, "%sSAMPLING_RATE", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_PKT_LEN_DISTRIB) fprintf(f, "%sPKT_LEN_DISTRIB", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_POST_NAT_SRC_HOST) fprintf(f, "%sPOST_NAT_SRC_IP", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_POST_NAT_DST_HOST) fprintf(f, "%sPOST_NAT_DST_IP", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_POST_NAT_SRC_PORT) fprintf(f, "%sPOST_NAT_SRC_PORT", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_POST_NAT_DST_PORT) fprintf(f, "%sPOST_NAT_DST_PORT", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_NAT_EVENT) fprintf(f, "%sNAT_EVENT", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_MPLS_LABEL_TOP) fprintf(f, "%sMPLS_LABEL_TOP", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_MPLS_LABEL_BOTTOM) fprintf(f, "%sMPLS_LABEL_BOTTOM", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_MPLS_STACK_DEPTH) fprintf(f, "%sMPLS_STACK_DEPTH", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_TIMESTAMP_START) fprintf(f, "%sTIMESTAMP_START", write_sep(sep, &count));
  if (config.what_to_count_2 & COUNT_TIMESTAMP_END) fprintf(f, "%sTIMESTAMP_END", write_sep(sep, &count));

  /* all custom primitives printed here */
  { 
    char cp_str[SRVBUFLEN];
    int cp_idx;

    for (cp_idx = 0; cp_idx < config.cpptrs.num; cp_idx++) {
      custom_primitive_header_print(cp_str, SRVBUFLEN, &config.cpptrs.primitive[cp_idx], FALSE);
      fprintf(f, "%s%s", write_sep(sep, &count), cp_str);
    }
  }

  if (!is_event) {
    fprintf(f, "%sPACKETS", write_sep(sep, &count));
    if (config.what_to_count & COUNT_FLOWS) fprintf(f, "%sFLOWS", write_sep(sep, &count));
    fprintf(f, "%sBYTES\n", write_sep(sep, &count));
  }
  else fprintf(f, "\n");
}
