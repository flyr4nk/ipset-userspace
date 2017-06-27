/*******************************************************************************
  版权说明：
  文 件 名: ip_set_hash_net.c
  创 建 人: lux               
  创建日期: 2017-5-27
  文件描述: ipset ipv4地址集接口实现文件，支持地址范围，网络地址，单个地址
  其    它: 
  修改记录: 
  1. 2017-5-27 create by lux
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ip_set_hash_net.h"
#include "ip_range.h"

#define HOST_MASK 32

static inline bool hash_net4_data_equal(
			const struct hash_net4_elem *ip1,
		    const struct hash_net4_elem *ip2,
		    u32 *multi)
{
	return ip1->ip == ip2->ip &&
	       ip1->cidr == ip2->cidr;
}

static inline void hash_net4_data_netmask(struct hash_net4_elem *elem, u8 cidr)
{
	elem->ip &= ip_set_netmask(cidr);
	elem->cidr = cidr;
}

int hash_net4_create(struct hash_net4 **h,u32 htable_size, u32 flags)
{
	struct hash_net4 *map;
	struct htable *t;
	u32 i = 0;
	u32 htable_bits;
	u32 hbucket_cnt;
	//struct hbucket *n;

	if(0 == htable_size)
	{
		htable_size = HTABLE_DEFAULT_SIZE;
	}
	
	htable_bits = get_hashbits(htable_size);
	hbucket_cnt = pow(2,htable_bits);

	map = (struct hash_net4*)malloc(sizeof(struct hash_net4));
	if(NULL == map)
	{
		DP("malloc hash_net4 error!\n");
		return -1;
	}

	memset((void*)map,0,sizeof(struct hash_net4));
	
	srand((int)time(NULL));
	map->maxelem = hbucket_cnt*HBUCKET_INIT_ELEM;
	map->initval = (u32)rand(); // 0x30303030;
	map->netmask = 0;
	map->elements = 0;

	map->table = (struct htable*)malloc(sizeof(struct htable));
	if(NULL == map->table)
	{
		DP("malloc hash_net4 htable error!\n");
		free(map);
		return -1;
	}

	memset((void*)map->table,0,sizeof(struct htable));
	
	t = map->table;
	t->htable_bits = htable_bits;
	t->htable_size = htable_size;

	t->bucket = malloc(hbucket_cnt * sizeof(struct hbucket));
	if(NULL == t->bucket)
	{
		free(map->table);
		free(map);
		return -1;
	}

	memset((void*)t->bucket,0,hbucket_cnt * sizeof(struct hbucket));
	
	for(;i<hbucket_cnt;i++)
	{
		t->bucket[i].size = HBUCKET_INIT_ELEM;
		t->bucket[i].pos = 1;
		t->bucket[i].value = malloc(sizeof(struct hash_net4_elem) * HBUCKET_INIT_ELEM);
		if(t->bucket[i].value != NULL)
		{
			memset((void*)t->bucket[i].value,0,sizeof(struct hash_net4_elem) * HBUCKET_INIT_ELEM);
		}
		else
		{
			DP("malloc bucket[%u].value error!\n",i);
		}
	}

	*h = map;

	DP("hash_net4_create: hash_size=%u htable_bits=%u initval=%u\n",
		htable_size,t->htable_bits,map->initval);

	return 0;
}

int hash_net4_add(struct hash_net4 *h,struct hash_net4_elem *elem)
{
	u32 key;
	struct hbucket *n;
	unsigned int i = 0;
	int ret = -1;

	if(h->elements >= h->maxelem)
	{
		DP("hash_net4_add err table full elements=%u,so update htable\n", h->elements);

		hash_net4_expire(h);
		if(h->elements >= h->maxelem)
		{
			return -1;
		}
	}

	ret = hash_net4_test(h,elem);
	if(ret > 0)
	{
		DP("hash_net4_add err ip existed: ip=%s ret=%u\n", inet_ntoa(*(struct in_addr*)(&elem->ip)),ret);
		return -2;
	}

	if(elem->cidr > HOST_MASK || elem->cidr == 0)
	{
		DP("hash_net4_add netmask err cidr=%u ret=%u\n", elem->cidr,ret);
		return -1;
	}

	unsigned int ipv4 = elem->ip;
	unsigned int cidr_idx = elem->cidr - 1;

	//key = hkey(&ipv4, h->initval, h->table->htable_bits);
	key = jhash_1word(ipv4, h->initval)&jhash_mask(h->table->htable_bits);
	//DP("hash_net4_add: key=%u ipv4=%u h->initval=%u htable_bits=%u\n"
	//			,key,ipv4,h->initval, h->table->htable_bits);
	n =  &h->table->bucket[key];

	if(NULL == n)
	{
		return -1;
	}

	struct hash_net4_elem *array =  (struct hash_net4_elem *)n->value;
	for (i = 0; i < n->size; i++) 
	{
		if (!test_bit(i,&n->used)) 
		{
			set_bit(i,&n->used);
			memcpy(&array[i],elem,sizeof(struct hash_net4_elem));
			h->elements++;
			ret = 0;
			
			h->nets[cidr_idx].nets[0]++;
			if(0 == h->nets[cidr_idx].cidr[0])
			{
				h->nets[cidr_idx].cidr[0] = elem->cidr;
			}
			
			if(n->pos < n->size)
			{
				n->pos++;
			}
			DP("hash_net4_add(%s) key=%u ok: nets=%d cidr=%d\n",
				inet_ntoa(*(struct in_addr*)(&array[i].ip)),key,
				h->nets[cidr_idx].nets[0],h->nets[cidr_idx].cidr[0]);
			break;
		}
	}

	if(i == n->size)
	{
		DP("hash_net4_add err table full: key=%u ipv4=%u h->initval=%u htable_bits=%u\n"
				,key,ipv4,h->initval, h->table->htable_bits);
	}

	return ret;
}

int hash_net4_add_ip(struct hash_net4 *h,u32 ip)
{
	struct hash_net4_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	memset(&elem,0,sizeof(struct hash_net4_elem));
	elem.ip = ip;
	elem.cidr = HOST_MASK;

	int ret = hash_net4_add(h,&elem);

	return ret;
}

int hash_net4_add_net(struct hash_net4 *h,u32 ip,u8 cidr)
{
	struct hash_net4_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	if(0 == cidr || cidr > HOST_MASK)
	{
		return -1;
	}
	
	memset(&elem,0,sizeof(struct hash_net4_elem));
	ip &= ip_set_netmask(cidr);
	
	elem.ip = ip;
	elem.cidr = cidr;

	int ret = hash_net4_add(h,&elem);

	return ret;
}

int hash_net4_add_iprange(struct hash_net4 *h,u32 ip_start,u32 ip_end)
{
	int ret = 0;
	u32 last;
	unsigned char tmp_cidr = 0;
	unsigned int tmp_addr = 0;
	
	if(NULL == h)
	{
		return -1;
	}

	if(ip_start == ip_end)
	{
		return hash_net4_add_ip(h,ip_start);
	}

	ip_start = ntohl(ip_start);
	ip_end = ntohl(ip_end);

	if(ip_start > ip_end)
	{
		int tmp = ip_start;
		ip_start = ip_end;
		ip_end = tmp;
	}

	if((ip_end - ip_start) > (1<<24))
	{
		DP("hash_net4_add_iprange: ip out of range(2^24=%d)\n",1<<24);
		return -1;
	}
	
	while (ip_start <= ip_end)
	{
		tmp_addr = htonl(ip_start);
				
		last = ip_set_range_to_cidr(ip_start, ip_end, &tmp_cidr);

		if(tmp_cidr == HOST_MASK)
		{
			ret = hash_net4_add_ip(h,tmp_addr);
		}
		else
		{
			ret = hash_net4_add_net(h, tmp_addr,tmp_cidr);
		}

		if(ret < 0)
		{
			break;
		}

		DP("hash_net4_add_iprange: ip(%s) cidr(%d)\n",
				inet_ntoa(*(struct in_addr*)(&tmp_addr)),tmp_cidr);
			
		ip_start = last + 1;
	}

	return ret;
}

int hash_net4_add_ip_timeout(struct hash_net4 *h,u32 ip,u32 timeout)
{
	struct hash_net4_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	memset(&elem,0,sizeof(struct hash_net4_elem));
	elem.ip = ip;
	elem.cidr = HOST_MASK;
	elem.lifetime = time(NULL) + timeout;

	int ret = hash_net4_add(h,&elem);

	return ret;
}

int hash_net4_del(struct hash_net4 *h,struct hash_net4_elem* elem)
{
	u32 key;
	struct hbucket *n;
	unsigned int i = 0;
	int ret = -1;
	struct hash_net4_elem *array;

	unsigned int ipv4 = elem->ip;
	unsigned int cidr_idx = elem->cidr - 1;

	if(elem->cidr > HOST_MASK)
	{
		DP("hash_net4_add netmask err cidr=%u ret=%u\n", elem->cidr,ret);
		return -1;
	}
	
	//key = hkey(&ipv4, h->initval, h->table->htable_bits);
	key = jhash_1word(ipv4, h->initval)&jhash_mask(h->table->htable_bits);
	//DP("hash_net4_test: key=%u ipv4=%u h->initval=%u htable_bits=%u\n"
		//		,key,ipv4,h->initval, h->table->htable_bits);

	n = &h->table->bucket[key];
	if(NULL == n)
	{
		return -1;
	}

	array =  (struct hash_net4_elem *)n->value;
	for (i = 0; i<n->pos; i++)
	{
		if (!test_bit(i, &n->used))
			continue;
		if(hash_net4_data_equal(&array[i],elem,NULL))
		{
			clear_bit(i, &n->used);
			memset(&array[i],0,sizeof(struct hash_net4_elem));
			h->elements--;
			ret = 0;

			h->nets[cidr_idx].nets[0]--;
			if(0 == h->nets[cidr_idx].nets[0])
			{
				h->nets[cidr_idx].cidr[0] = 0;
			}

			if((i + 1) == n->pos)
			{
				n->pos--;
			}
			
			break;
		}
	}

	return ret;
}

int hash_net4_del_ip(struct hash_net4 *h,u32 ip)
{
	struct hash_net4_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	memset(&elem,0,sizeof(struct hash_net4_elem));
	elem.ip = ip;
	elem.cidr = HOST_MASK;

	int ret = hash_net4_del(h,&elem);

	return ret;
}


int hash_net4_test(struct hash_net4 *h,struct hash_net4_elem* elem)
{
	u32 key;
	struct hbucket *n;
	unsigned int i, j = 0;
	int ret = -1;
	unsigned int ipv4;
	struct hash_net4_elem *array;
	struct hash_net4_elem tmp;

	if(NULL == h || NULL == elem)
	{
		return -1;
	}

	//unsigned int ipv4 = elem->ip;
	//key = hkey(&ipv4, h->initval, h->table->htable_bits);
	
	//DP("hash_net4_test: key=%u ipv4=%u h->initval=%u htable_bits=%u\n"
		//		,key,ipv4,h->initval, h->table->htable_bits);

	for (j=0 ; j < NLEN ; j++)//&& h->nets[j].nets[0]
	{
		if(h->nets[j].nets[0] == 0)
		{
			//DP("***hash_net4_test1: nets=%u cidr=%u\n",h->nets[j].nets[0],h->nets[j].cidr[0]);
			continue;
		}
		
		memcpy(&tmp,elem,sizeof(struct hash_net4_elem));
		hash_net4_data_netmask(&tmp,h->nets[j].cidr[0]);
		ipv4 = tmp.ip;

		//DP("***hash_net4_test2: ipv4=%s cidr=%u\n",
			//inet_ntoa(*(struct in_addr*)(&ipv4)),tmp.cidr);
		
		key = jhash_1word(ipv4, h->initval)&jhash_mask(h->table->htable_bits);
		n =  &h->table->bucket[key];
		if(NULL == n)
		{
			ret = -1;
			break;
		}
	
		array =  (struct hash_net4_elem *)n->value;
		for (i = 0; i<n->pos; i++)
		{
			if (!test_bit(i, &n->used))
				continue;
			if(hash_net4_data_equal(&array[i],&tmp,NULL))
			{
				if(array[i].lifetime != 0 && array[i].lifetime < time(NULL))
				{
					DP("hash_net4_test(key=%u) timeout,so del ip=%s\n",
						key,inet_ntoa(*(struct in_addr*)(&array[i].ip)));
					clear_bit(i, &n->used);
					memset(&array[i],0,sizeof(struct hash_net4_elem));
					ret = -2;
					h->elements--;
				}
				else
				{
					ret = 1;
				}
				return ret;
			}
		}
	}
	return ret;
}

int hash_net4_test_ip(struct hash_net4 *h,u32 ip)
{
	struct hash_net4_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	memset(&elem,0,sizeof(struct hash_net4_elem));
	elem.ip = ip;
	elem.cidr = HOST_MASK;

	int ret = hash_net4_test(h,&elem);

	return ret;
}

int hash_net4_list(struct hash_net4 *h)
{
	u32 i,j;
	struct hbucket *n;
	//struct hash_net4_elem* elem;
	struct hash_net4_elem *array;

	if(NULL == h)
	{
		return -1;
	}

	DP("hash_net4_list total num = <%u>\n",h->elements);
	
	for(i=0;i<jhash_size(h->table->htable_bits);i++)
	{
		n =  &h->table->bucket[i];
		array =  (struct hash_net4_elem *)n->value;
		//DP("*****bucket[%u] start******\n",i);
		for(j=0;j<n->size;j++)
		{
			if (test_bit(j, &n->used))
			{
				DP("bucket[%u] elem[%u] ip=%u.%u.%u.%u cidr=%u\n",
					i,j,NIPQUAD(array[j].ip),array[j].cidr);
			}
		}
		//DP("*****bucket[%u] end******\n",i);
	}

	return 0;
}

int hash_net4_expire(struct hash_net4 *h)
{
	u32 i,j;
	struct hbucket *n;
	//struct hash_net4_elem* elem;
	struct hash_net4_elem *array;

	if(NULL == h)
	{
		return -1;
	}
	
	for(i=0;i<jhash_size(h->table->htable_bits);i++)
	{
		n =  &h->table->bucket[i];
		array =  (struct hash_net4_elem *)n->value;
		//DP("*****bucket[%u] start******\n",i);
		for(j=0;j<n->pos;j++)
		{
			if (test_bit(j, &n->used))
			{				
				if(array[j].lifetime != 0 && array[j].lifetime < time(NULL))
				{
					DP("bucket[%u] elem[%u] timeout delete: ip=%u.%u.%u.%u cidr=%u\n",
						i,j,NIPQUAD(array[j].ip),array[j].cidr);
					clear_bit(j, &n->used);
					memset(&array[j],0,sizeof(struct hash_net4_elem));
					h->elements--;
					
					if((j + 1) == n->pos)
					{
						n->pos--;
					}
				}
			}
		}
		//DP("*****bucket[%u] end******\n",i);
	}

	DP("hash_net4_expire total num = <%u>\n",h->elements);

	return 0;
}

int hash_net4_destory(struct hash_net4 *h)
{
	u32 i;
	struct hbucket *n;

	if(NULL == h)
	{
		return -1;
	}
	
	for(i=0;i<jhash_size(h->table->htable_bits);i++)
	{
		n = &h->table->bucket[i];
		if(NULL != n && NULL != n->value)
		{
			free(n->value);
		}
	}

	free(h->table->bucket);
	free(h->table);
	free(h);
	
	return 0;
}


