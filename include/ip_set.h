#ifndef _IP_SET_H
#define _IP_SET_H

#include <string.h>

#define MAX_RANGE 0x0000FFFF
#define UINT_MAX	(~0U)
#define IP_SET_MAXNAMELEN 32	/* set names and set typenames */

#define initval_t uint32_t

typedef unsigned int uint32_t;
typedef unsigned int __u32;
typedef unsigned int u32;
typedef unsigned int u_int32_t;


typedef unsigned short u_int16_t;
typedef unsigned short uint16_t;
typedef unsigned short u16;

typedef unsigned char u8;

typedef uint32_t ip_set_ip_t;
typedef uint16_t ip_set_id_t;

typedef int bool;



/* A generic IP set */
struct ip_set {
	/* The name of the set */
	char name[IP_SET_MAXNAMELEN];
	/* Lock protecting the set data */
	//spinlock_t lock;

	ip_set_id_t id;			/* set id for swapping */
	
	/* References to the set */
	u32 ref;
	/* References to the set for netlink events like dump,
	 * ref can be swapped out by ip_set_swap
	 */
	u32 ref_netlink;
	/* The core set type */
	//struct ip_set_type *type;
	/* The type variant doing the real job */
	//const struct ip_set_type_variant *variant;
	/* The actual INET family of the set */
	u8 family;
	/* The type revision */
	u8 revision;
	/* Extensions */
	u8 extensions;
	/* Create flags */
	u8 flags;
	/* Default timeout value, if enabled */
	u32 timeout;
	/* Number of elements (vs timeout) */
	u32 elements;
	/* Size of the dynamic extensions (vs timeout) */
	u32 ext_size;
	/* Element data size */
	u32 dsize;
	/* Offsets to extensions in elements */
	//size_t offset[IPSET_EXT_ID_MAX];
	/* The type specific data */
	void *data;
};

#define DEBUG_IPSET
#ifdef DEBUG_IPSET
#define DP(format, args...) 					\
	do {							\
		printf("(DBG)File=%s: Func=%s: ", __FILE__, __FUNCTION__);\
		printf(format "" , ## args);			\
	} while (0)
#else
#define DP(format, args...)
#endif 

#define jhash_ip(map, i, ip)	jhash_1word(ip, *(map->initval + i))

#ifndef NIPQUAD
#define NIPQUAD(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]
#endif

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN
#endif

#ifndef HIPQUAD
#if defined(__LITTLE_ENDIAN)
#define HIPQUAD(addr) \
	((unsigned char *)&addr)[3], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[0]
#elif defined(__BIG_ENDIAN)
#define HIPQUAD NIPQUAD
#else
#error "Please fix asm/byteorder.h"
#endif /* __LITTLE_ENDIAN */
#endif

/* Error codes */
enum ipset_errno {
	IPSET_ERR_PRIVATE = 4096,
	IPSET_ERR_PROTOCOL,
	IPSET_ERR_FIND_TYPE,
	IPSET_ERR_MAX_SETS,
	IPSET_ERR_BUSY,
	IPSET_ERR_EXIST_SETNAME2,
	IPSET_ERR_TYPE_MISMATCH,
	IPSET_ERR_EXIST,
	IPSET_ERR_INVALID_CIDR,
	IPSET_ERR_INVALID_NETMASK,
	IPSET_ERR_INVALID_FAMILY,
	IPSET_ERR_TIMEOUT,
	IPSET_ERR_REFERENCED,
	IPSET_ERR_IPADDR_IPV4,
	IPSET_ERR_IPADDR_IPV6,
	IPSET_ERR_COUNTER,
	IPSET_ERR_COMMENT,
	IPSET_ERR_INVALID_MARKMASK,
	IPSET_ERR_SKBINFO,

	/* Type specific error codes */
	IPSET_ERR_TYPE_SPECIFIC = 4352,
};

static inline int test_bit(int nr, const volatile unsigned int * addr)
{
	return ((1UL << (nr & 31)) & (((const volatile unsigned int *) addr)[nr >> 5])) != 0;
}

static inline void set_bit(unsigned long nr, volatile unsigned int *addr)
{
    volatile unsigned int *a = addr;
    int mask;
    a += nr >> 5;
    mask = 1 << (nr & 31);
    *a |= mask;
}

static inline void clear_bit(unsigned long nr, volatile unsigned int *addr)
{
    volatile unsigned int *a = addr;
	int mask;
    a += nr >> 5;
	mask = 1 << (nr & 31);
	*a &= ~mask;
}

#endif

