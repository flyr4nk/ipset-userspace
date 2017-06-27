#ifndef LIBIPSET_NF_INET_ADDR_H
#define LIBIPSET_NF_INET_ADDR_H

struct in_addr_t {
	unsigned int	s_addr;
};

struct in6_addr_t {
	union 
	{
		unsigned char		u6_addr8[16];
		unsigned short		u6_addr16[8];
		unsigned int		u6_addr32[4];
	} in6_u;
//#define s6_addr			in6_u.u6_addr8
//#define s6_addr16		in6_u.u6_addr16
//#define s6_addr32		in6_u.u6_addr32
};


/* The structure to hold IP addresses, same as in linux/netfilter.h */
union nf_inet_addr {
	unsigned int all[4];
	unsigned int ip;
	unsigned int ip6[4];
	struct in_addr_t  in;
	struct in6_addr_t in6;
};

#endif

