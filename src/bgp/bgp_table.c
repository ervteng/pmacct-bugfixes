/*
    pmacct (Promiscuous mode IP Accounting package)
    pmacct is Copyright (C) 2003-2014 by Paolo Lucente
*/

/* 
 Originally based on Quagga BGP routing table which is:

 Copyright (C) 1998, 2001 Kunihiro Ishiguro

GNU Zebra is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

GNU Zebra is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Zebra; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

/* defines */
#define __BGP_TABLE_C

/* includes */
#include "pmacct.h"
#include "bgp.h"

static void bgp_node_delete (struct bgp_node *);
static void bgp_node_free_aggressive (struct bgp_node *);
static void bgp_table_free (struct bgp_table *);

struct bgp_table *
bgp_table_init (afi_t afi, safi_t safi)
{
  struct bgp_table *rt;

  rt = malloc (sizeof (struct bgp_table));
  if (rt) {
    memset (rt, 0, sizeof (struct bgp_table));

    rt->type = BGP_TABLE_MAIN;
    rt->afi = afi;
    rt->safi = safi;
  }
  else {
    Log(LOG_ERR, "ERROR ( %s/core/BGP ): malloc() failed (bgp_table_init). Exiting ..\n", config.name);
    exit_all(1);
  }
  
  return rt;
}

void
bgp_table_finish (struct bgp_table **rt)
{
  bgp_table_free (*rt);
  *rt = NULL;
}

static struct bgp_node *
bgp_node_create ()
{
  struct bgp_node *rn;

  rn = (struct bgp_node *) malloc (sizeof (struct bgp_node));
  if (rn) {
    memset (rn, 0, sizeof (struct bgp_node));

    rn->info = (void **) malloc(sizeof(struct bgp_info *) * (config.bgp_table_peer_buckets * config.bgp_table_per_peer_buckets));
    if (rn->info) memset (rn->info, 0, sizeof(struct bgp_info *) * (config.bgp_table_peer_buckets * config.bgp_table_per_peer_buckets));
    else goto malloc_failed;
  }
  else goto malloc_failed;

  return rn;

  malloc_failed:
  Log(LOG_ERR, "ERROR ( %s/core/BGP ): malloc() failed (bgp_node_create). Exiting ..\n", config.name);
  exit_all(1);
}

/* Allocate new route node with prefix set. */
static struct bgp_node *
bgp_node_set (struct bgp_table *table, struct prefix *prefix)
{
  struct bgp_node *node;
  
  node = bgp_node_create ();

  prefix_copy (&node->p, prefix);
  node->table = table;

  return node;
}

/* Free route node. */
static void
bgp_node_free (struct bgp_node *node)
{
  free (node->info);
  free (node);
}

/* Free route node aggressively: also attributes and info;
   should be meant to be invoked only by bgp_table_free() */
static void
bgp_node_free_aggressive (struct bgp_node *node)
{
  struct bgp_info *ri, *next;
  u_int32_t ri_idx;

  for (ri_idx = 0; ri_idx < (config.bgp_table_peer_buckets * config.bgp_table_per_peer_buckets); ri_idx++) {
    for (ri = node->info[ri_idx]; ri; ri = next) {
      if (config.nfacctd_bgp_msglog) {
        char empty[] = "";
        char prefix_str[INET6_ADDRSTRLEN];
        char *aspath, *comm, *ecomm;

        memset(prefix_str, 0, INET6_ADDRSTRLEN);
        prefix2str(&node->p, prefix_str, INET6_ADDRSTRLEN);

        aspath = ri->attr->aspath ? ri->attr->aspath->str : empty;
        comm = ri->attr->community ? ri->attr->community->str : empty;
        ecomm = ri->attr->ecommunity ? ri->attr->ecommunity->str : empty;

        Log(LOG_INFO, "INFO ( %s/core/BGP ): d Prefix: %s Path: '%s' Comms: '%s' EComms: '%s'\n",
			config.name, prefix_str, aspath, comm, ecomm);
      }

      next = ri->next;
      bgp_info_free(ri);
    }
  }

  free (node->info);
  free (node);
}

/* Free route table. */
static void
bgp_table_free (struct bgp_table *rt)
{
  struct bgp_node *tmp_node;
  struct bgp_node *node;
 
  if (rt == NULL)
    return;

  node = rt->top;

  while (node)
    {
      if (node->l_left)
	{
	  node = node->l_left;
	  continue;
	}

      if (node->l_right)
	{
	  node = node->l_right;
	  continue;
	}

      tmp_node = node;
      node = node->parent;

      if (node != NULL)
	{
	  if (node->l_left == tmp_node)
	    node->l_left = NULL;
	  else
	    node->l_right = NULL;

	  bgp_node_free_aggressive (tmp_node);
	}
      else
	{
	  bgp_node_free_aggressive (tmp_node);
	  break;
	}
    }
 
  free (rt);
  return;
}

/* Utility mask array. */
static u_char maskbit[] = 
{
  0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

/* Common prefix route genaration. */
static void
route_common (struct prefix *n, struct prefix *p, struct prefix *new)
{
  int i;
  u_char diff;
  u_char mask;

  u_char *np = (u_char *)&n->u.prefix;
  u_char *pp = (u_char *)&p->u.prefix;
  u_char *newp = (u_char *)&new->u.prefix;

  for (i = 0; i < p->prefixlen / 8; i++)
    {
      if (np[i] == pp[i])
	newp[i] = np[i];
      else
	break;
    }

  new->prefixlen = i * 8;

  if (new->prefixlen != p->prefixlen)
    {
      diff = np[i] ^ pp[i];
      mask = 0x80;
      while (new->prefixlen < p->prefixlen && !(mask & diff))
	{
	  mask >>= 1;
	  new->prefixlen++;
	}
      newp[i] = np[i] & maskbit[new->prefixlen % 8];
    }
}

/* Macro version of check_bit (). */
#define CHECK_BIT(X,P) ((((u_char *)(X))[(P) / 8]) >> (7 - ((P) % 8)) & 1)

/* Check bit of the prefix. */
static int
check_bit (u_char *prefix, u_char prefixlen)
{
  int offset;
  int shift;
  u_char *p = (u_char *)prefix;

  assert (prefixlen <= 128);

  offset = prefixlen / 8;
  shift = 7 - (prefixlen % 8);
  
  return (p[offset] >> shift & 1);
}

/* Macro version of set_link (). */
#define SET_LINK(X,Y) (X)->link[CHECK_BIT(&(Y)->prefix,(X)->prefixlen)] = (Y);\
                      (Y)->parent = (X)

static void
set_link (struct bgp_node *node, struct bgp_node *new)
{
  int bit;
    
  bit = check_bit (&new->p.u.prefix, node->p.prefixlen);

  assert (bit == 0 || bit == 1);

  node->link[bit] = new;
  new->parent = node;
}

/* Lock node. */
struct bgp_node *
bgp_lock_node (struct bgp_node *node)
{
  node->lock++;
  return node;
}

/* Unlock node. */
void
bgp_unlock_node (struct bgp_node *node)
{
  node->lock--;

  if (node->lock == 0)
    bgp_node_delete (node);
}

/* Find matched prefix. */
struct bgp_node *
bgp_node_match (const struct bgp_table *table, struct prefix *p, struct bgp_peer *peer)
{
  struct bgp_node *node;
  struct bgp_node *matched;
  struct bgp_info *info;
  u_int32_t modulo = bgp_route_info_modulo(peer, NULL);

  matched = NULL;
  node = table->top;

  /* Walk down tree.  If there is matched route then store it to
     matched. */
  while (node && node->p.prefixlen <= p->prefixlen && prefix_match(&node->p, p)) {
    for (info = node->info[modulo]; info; info = info->next) {
      if (info->peer == peer) {
	matched = node;
        break;
      }
    }

    node = node->link[check_bit(&p->u.prefix, node->p.prefixlen)];
  }

  /* If matched route found, return it. */
  if (matched) return bgp_lock_node (matched);

  return NULL;
}

struct bgp_node *
bgp_node_match_ipv4 (const struct bgp_table *table, struct in_addr *addr, struct bgp_peer *peer)
{
  struct prefix_ipv4 p;

  memset (&p, 0, sizeof (struct prefix_ipv4));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.prefix = *addr;

  return bgp_node_match (table, (struct prefix *) &p, peer);
}

#ifdef ENABLE_IPV6
struct bgp_node *
bgp_node_match_ipv6 (const struct bgp_table *table, struct in6_addr *addr, struct bgp_peer *peer)
{
  struct prefix_ipv6 p;

  memset (&p, 0, sizeof (struct prefix_ipv6));
  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.prefix = *addr;

  return bgp_node_match (table, (struct prefix *) &p, peer);
}
#endif /* ENABLE_IPV6 */

/* Add node to routing table. */
struct bgp_node *
bgp_node_get (struct bgp_table *const table, struct prefix *p)
{
  struct bgp_node *new;
  struct bgp_node *node;
  struct bgp_node *match;

  match = NULL;
  node = table->top;
  while (node && node->p.prefixlen <= p->prefixlen && 
	 prefix_match (&node->p, p))
    {
      if (node->p.prefixlen == p->prefixlen)
	{
	  bgp_lock_node (node);
	  return node;
	}
      match = node;
      node = node->link[check_bit(&p->u.prefix, node->p.prefixlen)];
    }

  if (node == NULL)
    {
      new = bgp_node_set (table, p);
      if (match)
	set_link (match, new);
      else
	table->top = new;
    }
  else
    {
      new = bgp_node_create ();
      route_common (&node->p, p, &new->p);
      new->p.family = p->family;
      new->table = table;
      set_link (new, node);

      if (match)
	set_link (match, new);
      else
	table->top = new;

      if (new->p.prefixlen != p->prefixlen)
	{
	  match = new;
	  new = bgp_node_set (table, p);
	  set_link (match, new);
	  table->count++;
	}
    }
  table->count++;
  bgp_lock_node (new);
  
  return new;
}

/* Delete node from the routing table. */
static void
bgp_node_delete (struct bgp_node *node)
{
  struct bgp_node *child;
  struct bgp_node *parent;
  u_int32_t ri_idx;

  assert (node->lock == 0);
  for (ri_idx = 0; ri_idx < (config.bgp_table_peer_buckets * config.bgp_table_per_peer_buckets); ri_idx++)
    assert (node->info[ri_idx] == NULL);

  if (node->l_left && node->l_right)
    return;

  if (node->l_left)
    child = node->l_left;
  else
    child = node->l_right;

  parent = node->parent;

  if (child)
    child->parent = parent;

  if (parent)
    {
      if (parent->l_left == node)
	parent->l_left = child;
      else
	parent->l_right = child;
    }
  else
    node->table->top = child;
  
  node->table->count--;
  
  bgp_node_free (node);

  /* If parent node is stub then delete it also. */
  if (parent && parent->lock == 0)
    bgp_node_delete (parent);
}

/* Get fist node and lock it.  This function is useful when one want
   to lookup all the node exist in the routing table. */
struct bgp_node *
bgp_table_top (const struct bgp_table *const table)
{
  /* If there is no node in the routing table return NULL. */
  if (table->top == NULL)
    return NULL;

  /* Lock the top node and return it. */
  bgp_lock_node (table->top);
  return table->top;
}

/* Unlock current node and lock next node then return it. */
struct bgp_node *
bgp_route_next (struct bgp_node *node)
{
  struct bgp_node *next;
  struct bgp_node *start;

  /* Node may be deleted from bgp_unlock_node so we have to preserve
     next node's pointer. */

  if (node->l_left)
    {
      next = node->l_left;
      bgp_lock_node (next);
      bgp_unlock_node (node);
      return next;
    }
  if (node->l_right)
    {
      next = node->l_right;
      bgp_lock_node (next);
      bgp_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent)
    {
      if (node->parent->l_left == node && node->parent->l_right)
	{
	  next = node->parent->l_right;
	  bgp_lock_node (next);
	  bgp_unlock_node (start);
	  return next;
	}
      node = node->parent;
    }
  bgp_unlock_node (start);
  return NULL;
}

/* Unlock current node and lock next node until limit. */
struct bgp_node *
bgp_route_next_until (struct bgp_node *node, struct bgp_node *limit)
{
  struct bgp_node *next;
  struct bgp_node *start;

  /* Node may be deleted from bgp_unlock_node so we have to preserve
     next node's pointer. */

  if (node->l_left)
    {
      next = node->l_left;
      bgp_lock_node (next);
      bgp_unlock_node (node);
      return next;
    }
  if (node->l_right)
    {
      next = node->l_right;
      bgp_lock_node (next);
      bgp_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent && node != limit)
    {
      if (node->parent->l_left == node && node->parent->l_right)
	{
	  next = node->parent->l_right;
	  bgp_lock_node (next);
	  bgp_unlock_node (start);
	  return next;
	}
      node = node->parent;
    }
  bgp_unlock_node (start);
  return NULL;
}

unsigned long
bgp_table_count (const struct bgp_table *table)
{
  return table->count;
}
