#ifndef _IP_RANGE_H_
#define _IP_RANGE_H_

#include <arpa/inet.h>
#include "nf_inet_addr.h"


extern const union nf_inet_addr ip_set_netmask_map[];
extern const union nf_inet_addr ip_set_hostmask_map[];

static inline unsigned int
ip_set_hostmask(unsigned char pfxlen)
{
	return (unsigned int)ip_set_hostmask_map[pfxlen].ip;
}

static inline unsigned int
ip_set_netmask(unsigned char pfxlen)
{
	return (unsigned int)ip_set_netmask_map[pfxlen].ip;
}

static inline const unsigned int *
ip_set_netmask6(unsigned char pfxlen)
{
	return &ip_set_netmask_map[pfxlen].ip6[0];
}

extern unsigned int ip_set_range_to_cidr(unsigned int from, unsigned int to, unsigned char *cidr);

#endif

