#ifndef __IP_SET_HASH_NET_H
#define __IP_SET_HASH_NET_H

#include <math.h>
#include "ip_set.h"
#include "ip_set_jhash.h"
#include "nf_inet_addr.h"


//#define htype hash_net4

#define HTABLE_DEFAULT_SIZE		4096
#define HTABLE_MAX_BITS			30
#define HTABLE_BUCKET_MAX_CNT ({ pow(2,HTABLE_MAX_BITS); })

#define BUCKET_ELEM_MAX_CNT 	32
#define HBUCKET_INIT_ELEM		8


#define IPSET_NET_COUNT 2
#define NLEN 			32
#define NLEN_IPV6		128


/* Member elements  */
struct hash_net4_elem {
	u32 ip;
	u16 padding0;
	u8 	nomatch;
	u8 	cidr;
	unsigned long lifetime;
};

struct hash_net6_elem {
	union nf_inet_addr ip;
	u16 padding0;
	u8 nomatch;
	u8 cidr;
	unsigned long lifetime;
};

/* A hash bucket */
struct hbucket {
	//struct rcu_head rcu;	/* for call_rcu_bh */
	u32 used;
	u8 size;		/* size of the array */
	u8 pos;			/* position of the first free entry */
	//unsigned char value[0]	/* the array of the values */
	char* value;
};

struct htable {
	u32 ref;		/* References for resizing */
	u32 uref;		/* References for dumping */
	u8 htable_bits;		/* size of hash table == 2^htable_bits */
	u32 htable_size;
	//struct hbucket *bucket[0]; /* hashtable buckets */
	struct hbucket *bucket;
};

struct net_prefixes {
	u32 nets[IPSET_NET_COUNT]; /* number of elements for this cidr */
	u8 cidr[IPSET_NET_COUNT];  /* the cidr value */
};

/* The generic hash structure */
struct hash_net4 {
	struct htable *table; /* the hash table */
	//pthread_mutex_t hlock;
	//struct timer_list gc;	/* garbage collection when timeout enabled */
	u32 maxelem;		/* max elements in the hash */
	u32 initval;		/* random jhash init value */

	u32 markmask;		/* markmask value for mark mask to store */

	u8 ahash_max;		/* max elements in an array block */

	u8 netmask;		/* netmask value for subnets to store */

	//struct mtype_elem next; /* temporary storage for uadd */

	struct net_prefixes nets[NLEN]; /* book-keeping of prefixes */

	u32 elements;
};

struct hash_net6 {
	struct htable *table; /* the hash table */
	//pthread_mutex_t hlock;
	//struct timer_list gc;	/* garbage collection when timeout enabled */
	u32 maxelem;		/* max elements in the hash */
	u32 initval;		/* random jhash init value */

	u32 markmask;		/* markmask value for mark mask to store */

	u8 ahash_max;		/* max elements in an array block */

	u8 netmask;		/* netmask value for subnets to store */

	struct net_prefixes nets[NLEN_IPV6]; /* book-keeping of prefixes */

	u32 elements;
};


#ifndef HKEY_DATALEN
#define HKEY_DATALEN		sizeof(struct hash_net4_elem)
#endif

#define HKEY(data, initval, htable_bits)			\
({								\
	const u32 *__k = (const u32 *)data;			\
	u32 __l = HKEY_DATALEN / sizeof(u32);			\
								\
	jhash2(__k, __l, initval) & jhash_mask(htable_bits);	\
})

static inline u32 hkey(const u32 *data,u32 initval,u8 htable_bits)
{
	const u32 *__k = (const u32 *)data;			
	u32 __l = HKEY_DATALEN / sizeof(u32);								
	return jhash2(__k, __l, initval) & jhash_mask(htable_bits);	
}

static inline u32 get_hashbits(u32 hash_size)
{
    u32 bits = 0;
    while(hash_size >= 2)
    {
        hash_size = hash_size/2;
        bits++;
    }
    return bits;
}

int hash_net4_create(struct hash_net4 **h,u32 htable_size, u32 flags);

int hash_net4_add(struct hash_net4 *h,struct hash_net4_elem* elem);
int hash_net4_add_ip(struct hash_net4 *h,u32 ip);
int hash_net4_add_net(struct hash_net4 *h,u32 ip,u8 cidr);
int hash_net4_add_iprange(struct hash_net4 *h,u32 ip_start,u32 ip_end);
int hash_net4_add_ip_timeout(struct hash_net4 *h,u32 ip,u32 timeout);


int hash_net4_del(struct hash_net4 *h,struct hash_net4_elem* elem);
int hash_net4_del_ip(struct hash_net4 *h,u32 ip);

int hash_net4_test(struct hash_net4 *h,struct hash_net4_elem* elem);
int hash_net4_test_ip(struct hash_net4 *h,u32 ip);

int hash_net4_list(struct hash_net4 *h);

int hash_net4_expire(struct hash_net4 *h);
int hash_net4_destory(struct hash_net4 *h);


/********************************************IPV6******************************/

int hash_net6_create(struct hash_net6 **h,u32 htable_size, u32 flags);

int hash_net6_add(struct hash_net6 *h,struct hash_net6_elem* elem);
int hash_net6_add_ip(struct hash_net6 *h,void* ipv6);
int hash_net6_add_ip_timeout(struct hash_net6 *h,void* ipv6,u32 timeout);
int hash_net6_add_net(struct hash_net6 *h,void* ipv6,u8 cidr);

int hash_net6_del(struct hash_net6 *h,struct hash_net6_elem* elem);
int hash_net6_del_ip(struct hash_net6 *h,void* ipv6);

int hash_net6_test(struct hash_net6 *h,struct hash_net6_elem* elem);
int hash_net6_test_ip(struct hash_net6 *h,void* ipv6);

int hash_net6_expire(struct hash_net6 *h);
int hash_net6_destory(struct hash_net6 *h);

#endif
