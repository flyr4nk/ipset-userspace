/*******************************************************************************
  版权说明：
  文 件 名: ip_set_iphash.c
  创 建 人: lux               
  创建日期: 2017-5-27
  文件描述: ipset ipv4地址集接口实现文件
  其    它: 
  修改记录: 
  1. 2017-5-27 create by lux
*******************************************************************************/
#include <stdio.h>
#include "ip_set_jhash.h"
#include "ip_set_iphash.h"
#include "ip_set_malloc.h"

static int limit = MAX_RANGE;

static void get_random_bytes(void *buf, int nbytes)
{
	memset(buf,0x01,nbytes);
}

static inline __u32
iphash_id(struct ip_set *set, ip_set_ip_t ip)
{
	struct ip_set_iphash *map = set->data;
	__u32 id;
	u_int16_t i;
	ip_set_ip_t *elem;

	//ip &= map->netmask;	
	DP("set: %s, ip:%u.%u.%u.%u", set->name, HIPQUAD(ip));
	for (i = 0; i < map->probes; i++) {
		id = jhash_ip(map, i, ip) % map->hashsize;
		DP("hash key: %u", id);
		elem = HARRAY_ELEM(map->members, ip_set_ip_t *, id);
		if (*elem == ip)
		{
			DP("find id: %u ip=%u", id,ip);
			return id;
		}
		/* No shortcut - there can be deleted entries. */
	}
	return UINT_MAX;
}

inline int
iphash_test(struct ip_set *set, ip_set_ip_t ip)
{
	return (ip && iphash_id(set, ip) != UINT_MAX);
}

static inline int
__iphash_add(struct ip_set_iphash *map, ip_set_ip_t *ip)
{
	__u32 probe;
	u_int16_t i;
	ip_set_ip_t *elem, *slot = NULL;
	
	for (i = 0; i < map->probes; i++) {
		probe = jhash_ip(map, i, *ip) % map->hashsize;
		elem = HARRAY_ELEM(map->members, ip_set_ip_t *, probe);
		if (*elem == *ip)
			return -2;
		if (!(slot || *elem))
			slot = elem;
		/* There can be deleted entries, must check all slots */
	}
	if (slot) {
		*slot = *ip;
		map->elements++;
		return 0;
	}
	/* Trigger rehashing */
	return -1;
}

inline int
iphash_add(struct ip_set *set, ip_set_ip_t ip)
{
	struct ip_set_iphash *map = set->data;
	
	if (!ip || map->elements >= limit)
		return -1;

	//ip &= map->netmask;
	return __iphash_add(map, &ip);
}

inline int
iphash_del(struct ip_set *set, ip_set_ip_t ip)
{
	struct ip_set_iphash *map = set->data;
	ip_set_ip_t id, *elem;

	if (!ip)
		return -1;

	id = iphash_id(set, ip);
	if (id == UINT_MAX)
		return -2;
		
	elem = HARRAY_ELEM(map->members, ip_set_ip_t *, id);
	*elem = 0;
	map->elements--;

	return 0;
}

inline int
iphash_create(struct ip_set_iphash **set,u_int32_t hashsize, u_int32_t probes)
{
	uint16_t i;	
	struct ip_set_iphash *map;
	
	map = malloc(sizeof(struct ip_set_iphash) + probes * sizeof(initval_t));
	if (!map)
	{
		return -1;
	}
	for (i = 0; i < probes; i++)				
		get_random_bytes(((initval_t *) map->initval)+i, 4);
	
	map->elements = 0;						
	map->hashsize = hashsize;	
	map->probes = probes;
	map->resize = 1024;	

	map->members = harray_malloc(map->hashsize, sizeof(struct ip_set_iphash));
	if (!map->members) {						
		DP("out of memory for %zu bytes", map->hashsize * sizeof(struct ip_set_iphash));
		free(map);
		return -1;						
	}

	*set = map;
	
	return 0;
}


