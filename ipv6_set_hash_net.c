#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ip_set_hash_net.h"
#include "ip_range.h"

#define HOST_MASK	128

static inline int ipv6_addr_equal(const struct in6_addr_t *a1,
				  const struct in6_addr_t *a2)
{
	return (((a1->in6_u.u6_addr32[0] ^ a2->in6_u.u6_addr32[0]) |
		 (a1->in6_u.u6_addr32[1] ^ a2->in6_u.u6_addr32[1]) |
		 (a1->in6_u.u6_addr32[2] ^ a2->in6_u.u6_addr32[2]) |
		 (a1->in6_u.u6_addr32[3] ^ a2->in6_u.u6_addr32[3])) == 0);
}

static inline void	ip6_netmask(union nf_inet_addr *ip, u8 prefix)
{
	ip->ip6[0] &= ip_set_netmask6(prefix)[0];
	ip->ip6[1] &= ip_set_netmask6(prefix)[1];
	ip->ip6[2] &= ip_set_netmask6(prefix)[2];
	ip->ip6[3] &= ip_set_netmask6(prefix)[3];
}

static inline bool hash_net6_data_equal(
			const struct hash_net6_elem *ip1,
		    const struct hash_net6_elem *ip2,
		    u32 *multi)
{
	return ipv6_addr_equal(&ip1->ip.in6, &ip2->ip.in6) &&
	       ip1->cidr == ip2->cidr;
}

static inline void hash_net6_data_netmask(
			struct hash_net6_elem *elem, 
			u8 cidr)
{
	ip6_netmask(&elem->ip, cidr);
	elem->cidr = cidr;
}

int hash_net6_create(struct hash_net6 **h,u32 htable_size, u32 flags)
{
	struct hash_net6 *map;
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

	map = (struct hash_net6*)malloc(sizeof(struct hash_net6));
	if(NULL == map)
	{
		return -1;
	}
	
	memset((void*)map,0,sizeof(struct hash_net6));
	srand((int)time(NULL));
	map->maxelem = hbucket_cnt*HBUCKET_INIT_ELEM;
	map->initval = (u32)rand(); // 0x30303030;
	map->netmask = 0;
	map->elements = 0;

	map->table = malloc(sizeof(struct htable));
	if(NULL == map->table)
	{
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
	
	memset((void*)t->bucket,0,hbucket_cnt*sizeof(struct hbucket));
	for(;i<hbucket_cnt;i++)
	{
		t->bucket[i].size = HBUCKET_INIT_ELEM;
		t->bucket[i].pos = 1;
		t->bucket[i].value = malloc(sizeof(struct hash_net6_elem) * HBUCKET_INIT_ELEM);
		if(t->bucket[i].value != NULL)
		{
			memset((void*)t->bucket[i].value,0,sizeof(struct hash_net6_elem) * HBUCKET_INIT_ELEM);
		}
		else
		{
			DP("malloc bucket[%u].value error!\n",i);
		}
	}

	*h = map;

	DP("hash_net6_create: hash_size=%u htable_bits=%u initval=%u\n",
		htable_size,t->htable_bits,map->initval);

	return 0;
}

int hash_net6_add(struct hash_net6 *h,struct hash_net6_elem* elem)
{
	u32 key;
	struct hbucket *n;
	unsigned int i = 0;
	int ret = -1;

	if(h->elements >= h->maxelem)
	{
		DP("hash_net6_add err table full elements=%u,so update htable\n", h->elements);

		hash_net6_expire(h);
		if(h->elements >= h->maxelem)
		{
			return -1;
		}
	}

	ret = hash_net6_test(h,elem);
	if(ret > 0)
	{
		DP("hash_net6_add err ip existed: ret=%u\n",ret);
		return -2;
	}

	if(elem->cidr > HOST_MASK || elem->cidr == 0)
	{
		DP("hash_net6_add netmask err cidr=%u ret=%u\n", elem->cidr,ret);
		return -1;
	}


	unsigned int cidr_idx = elem->cidr - 1;
	
	if(elem->cidr != HOST_MASK)
	{
		ip6_netmask(&elem->ip, elem->cidr);
	}

	key = jhash2((u32*)&elem->ip,4,h->initval)&jhash_mask(h->table->htable_bits);
	DP("hash_net6_add key=%u\n",key);
	n =  &h->table->bucket[key];

	if(NULL == n)
	{
		return -1;
	}
	
	struct hash_net6_elem *array =  (struct hash_net6_elem *)n->value;
	for (i = 0; i < n->size; i++) 
	{
		if (!test_bit(i,&n->used)) 
		{
			set_bit(i,&n->used);
			memcpy(&array[i],elem,sizeof(struct hash_net6_elem));
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
			break;
		}
	}

	if(i == n->size)
	{
		DP("hash_net6_add err table full: key=%u h->initval=%u htable_bits=%u\n"
				,key,h->initval, h->table->htable_bits);
	}

	return ret;
}

int hash_net6_del(struct hash_net6 *h,struct hash_net6_elem* elem)
{
	u32 key;
	struct hbucket *n;
	unsigned int i = 0;
	int ret = -1;
	struct hash_net6_elem *array;

	//unsigned int ipv4 = elem->ip;
	unsigned int cidr_idx = elem->cidr - 1;

	if(elem->cidr > HOST_MASK)
	{
		DP("hash_net6_add netmask err cidr=%u ret=%u\n", elem->cidr,ret);
		return -1;
	}
	
	key = jhash2((u32*)&elem->ip,4,h->initval)&jhash_mask(h->table->htable_bits);

	n = &h->table->bucket[key];
	if(NULL == n)
	{
		return -1;
	}

	array =  (struct hash_net6_elem *)n->value;
	for (i = 0; i<n->pos; i++)
	{
		if (!test_bit(i, &n->used))
			continue;
		if(hash_net6_data_equal(&array[i],elem,NULL))
		{
			clear_bit(i, &n->used);
			memset(&array[i],0,sizeof(struct hash_net6_elem));
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

int hash_net6_del_ip(struct hash_net6 *h,void* ipv6)
{
	struct hash_net6_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	memset(&elem,0,sizeof(struct hash_net6_elem));
	memcpy(&elem.ip,ipv6,sizeof(union nf_inet_addr));
	elem.cidr = HOST_MASK;

	int ret = hash_net6_del(h,&elem);

	return ret;
}

int hash_net6_test(struct hash_net6 *h,struct hash_net6_elem* elem)
{
	u32 key;
	struct hbucket *n;
	unsigned int i, j = 0;
	int ret = -1;
	//unsigned int* ip;
	struct hash_net6_elem *array;
	struct hash_net6_elem tmp;
	char addr[128] = {0};

	if(NULL == h || NULL == elem)
	{
		return -1;
	}

	for (j=0 ; j < NLEN_IPV6 ; j++)//&& h->nets[j].nets[0]
	{
		if(h->nets[j].nets[0] == 0)
		{
			continue;
		}
		
		memcpy(&tmp,elem,sizeof(struct hash_net6_elem));
		hash_net6_data_netmask(&tmp,h->nets[j].cidr[0]);
		
		key = jhash2((u32*)&tmp.ip,4,h->initval)&jhash_mask(h->table->htable_bits);
		
		n =  &h->table->bucket[key];
		if(NULL == n)
		{
			ret = -1;
			break;
		}
	
		array =  (struct hash_net6_elem *)n->value;
		for (i = 0; i<n->pos; i++)
		{
			if (!test_bit(i, &n->used))
				continue;
			if(hash_net6_data_equal(&array[i],&tmp,NULL))
			{
				if(array[i].lifetime != 0 && array[i].lifetime < time(NULL))
				{
					inet_ntop(AF_INET6,(void*)&array[i].ip,addr,sizeof(addr));
					DP("hash_net6_test(key=%u) timeout,so del ip=%s\n",key,addr);
					clear_bit(i, &n->used);
					memset(&array[i],0,sizeof(struct hash_net6_elem));
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

int hash_net6_test_ip(struct hash_net6 *h, void* ipv6)
{
	struct hash_net6_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	memset(&elem,0,sizeof(struct hash_net6_elem));
	memcpy(&elem.ip,ipv6,sizeof(union nf_inet_addr));
	elem.cidr = HOST_MASK;

	int ret = hash_net6_test(h,&elem);

	return ret;
	return 0;
}


int hash_net6_add_ip(struct hash_net6 *h,void* ipv6)
{
	struct hash_net6_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	memset(&elem,0,sizeof(struct hash_net6_elem));
	memcpy(&elem.ip,ipv6,sizeof(union nf_inet_addr));
	elem.cidr = HOST_MASK;

	int ret = hash_net6_add(h,&elem);

	return ret;
}

int hash_net6_add_ip_timeout(struct hash_net6 *h,void* ipv6,u32 timeout)
{
	struct hash_net6_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	memset(&elem,0,sizeof(struct hash_net6_elem));
	memcpy(&elem.ip,ipv6,sizeof(union nf_inet_addr));
	elem.cidr = HOST_MASK;
	elem.lifetime = time(NULL) + timeout;

	int ret = hash_net6_add(h,&elem);

	return ret;
}

int hash_net6_add_net(struct hash_net6 *h,void* ipv6,u8 cidr)
{
	struct hash_net6_elem elem;
	
	if(NULL == h)
	{
		return -1;
	}

	memset(&elem,0,sizeof(struct hash_net6_elem));
	memcpy(&elem.ip,ipv6,sizeof(union nf_inet_addr));
	elem.cidr = cidr;
	ip6_netmask(&elem.ip, elem.cidr);

	int ret = hash_net6_add(h,&elem);

	return ret;
}

int hash_net6_expire(struct hash_net6 *h)
{
	u32 i,j;
	struct hbucket *n;
	struct hash_net6_elem *array;

	if(NULL == h)
	{
		return -1;
	}
	
	for(i=0;i<jhash_size(h->table->htable_bits);i++)
	{
		n =  &h->table->bucket[i];
		array =  (struct hash_net6_elem *)n->value;
		//DP("*****bucket[%u] start******\n",i);
		for(j=0;j<n->pos;j++)
		{
			if (test_bit(j, &n->used))
			{				
				if(array[j].lifetime != 0 && array[j].lifetime < time(NULL))
				{
					
					clear_bit(j, &n->used);
					memset(&array[j],0,sizeof(struct hash_net6_elem));
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

	DP("hash_net6_expire total num = <%u>\n",h->elements);

	return 0;
}

int hash_net6_destory(struct hash_net6 *h)
{
	u32 i;
	struct hbucket *n;

	if(NULL == h)
	{
		return -1;
	}
	
	for(i=0;i<jhash_size(h->table->htable_bits);i++)
	{
		n =  &h->table->bucket[i];

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

