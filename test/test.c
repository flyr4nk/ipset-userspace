#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ip_set.h"
#include "ip_set_iphash.h"
#include "ip_set_hash_net.h"

#define TEST_IPV6

int main()
{
	printf("hello ipset!\n");

#ifdef IPHASH_TEST
	struct ip_set_iphash *table;

	int ret = iphash_create(&table,1024,4);
	printf("iphash_create ret=%d table=%p!\n",ret,table);

	unsigned int ip = 0;
	struct ip_set set;
	set.data = table;

	
	int i = 0;
	for(;i<65535;i++)
	{
		ret = iphash_add(&set,++ip);
		if(ret != 0)
		{
			printf("iphash_add(i=%d) err ret=%d table=%p!\n",i,ret,table);
		}
	}

	ip = 65534;
	ret = iphash_test(&set,ip);
	printf("iphash_test(ip=%u) ret=%d table=%p!\n",ip,ret,table);
	ret = iphash_test(&set,++ip);
	printf("iphash_test(ip=%u) ret=%d table=%p!\n",ip,ret,table);
	
	return 0;
#endif 

#ifdef TEST_IPV6
	struct hash_net6 *htable; 
	int ret =  hash_net6_create(&htable,4096,0);
	printf("hash_net6_create ret=%d table=%p!\n",ret,htable);

	
	struct hash_net6_elem ip_elem;
	memset(&ip_elem,0,sizeof(struct hash_net6_elem));
	int i = 0;
	for(i=0;i<8;i++)
	{
		ip_elem.ip.ip6[0] = i;
		ip_elem.ip.ip6[1] = i;
		ip_elem.ip.ip6[2] = i;
		ip_elem.ip.ip6[3] = i;

		ip_elem.cidr = 64;

		ret = hash_net6_add(htable,&ip_elem);
		if(ret != 0)
		{
			printf("hash_net6_add(i=%d) err ret=%d table=%p!\n",i,ret,htable);
		}
	}

	//ret = hash_net6_del(htable,&ip_elem);

	clock_t  start, end;
    double  cpu_time_used;

	start = clock();
	for(i=0;i<1;i++)
    {
    	ip_elem.cidr = 128;
		ret = hash_net6_test(htable,&ip_elem);
		
	}
	end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("test time(loop=%u): %18.6lf\n",i,cpu_time_used);
	printf("hash_net6_test(ip=%u) ret=%d table=%p!\n",ip_elem.ip.ip6[0],ret,htable);
	
	hash_net6_expire(htable);

	hash_net6_destory(htable);

	return 0;
	
#endif

#ifdef TEST_IPV4
	struct hash_net4 *htable; 
	int ret =  hash_net4_create(&htable,4096,0);
	printf("hash_net4_create ret=%d table=%p!\n",ret,htable);

	struct hash_net4_elem ip_elem;
	int i = 0;

	memset(&ip_elem,0,sizeof(struct hash_net4_elem));
	for(i=0;i<8;i++)
	{
		ip_elem.ip = i;
		//ret = hash_net4_add(htable,&ip_elem);
		ret = hash_net4_add_ip_timeout(htable,i,4);
		if(ret != 0)
		{
			printf("hash_net4_add(i=%d) err ret=%d table=%p!\n",i,ret,htable);
		}
	}

	unsigned int ip_l = inet_addr("192.168.1.1");
	unsigned int ip_h = inet_addr("192.168.8.8");

	ret = hash_net4_add_iprange(htable,ip_l,ip_h);
	printf("hash_net4_add_iprange err ret=%d table=%p!\n",ret,htable);

	unsigned int ip_net = inet_addr("222.168.1.1");
	ret = hash_net4_add_net(htable,ip_net,24);
	printf("hash_net4_add_net err ret=%d table=%p!\n",ret,htable);

	/*for(i=0;i<4;i++)
	{
		ip_elem.ip = i;
		ret = hash_net4_del_ip(htable,ip_elem.ip);
		if(ret != 0)
		{
			printf("hash_net4_del_ip(i=%d) err ret=%d table=%p!\n",i,ret,htable);
		}
	}*/

	clock_t  start, end;
    double  cpu_time_used;

	start = clock();
	for(i=0;i<10000000;i++)
    {
       	//ip_elem.ip = 100;
		ip_elem.ip = inet_addr("222.168.1.10");
		ip_elem.cidr = 0;
		//ret = hash_net4_test(htable,&ip_elem);
		ret = hash_net4_test_ip(htable,ip_elem.ip);
		//printf("hash_net4_test(ip=%u) ret=%d table=%p!\n",ip_elem.ip,ret,htable);
    }

	end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("%18.6lf\n",cpu_time_used);

	printf("hash_net4_test(ip=%u) ret=%d table=%p!\n",ip_elem.ip,ret,htable);

	hash_net4_list(htable);

	hash_net4_expire(htable);

	hash_net4_destory(htable);
	

	//test2: test create free
	/*
	printf("hash_net4_create&free test start\n");
	for(i=0;i<10000;i++)
	{
		int ret =  hash_net4_create(&htable,1024,0);
		hash_net4_destory(htable);
	}
	printf("hash_net4_create&free test over\n");
	*/
	return 0;
#endif

}
