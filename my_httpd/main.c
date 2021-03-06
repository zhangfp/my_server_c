/*
author:zhangfp
date:2018-07-04
desciption: create a server frame by c and can be reused.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define LISTEN_PORT			80
#define LISTEN_QUEUE_NUM	102400

int set_sock_nonblock(int sockfd)
{
	fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
	return 0;
}

/*
判断是否是ipv4映射的ipv6地址
返回0：ipv6
返回1：ipv4
*/
int is_v4_mapped_ipv6(struct in6_addr in6_addr)
{
	return (in6_addr.s6_addr32[0] == 0 && in6_addr.s6_addr32[1] == 0 && in6_addr.s6_addr32[2] == htonl(0x0000ffff));
}

int get_str_ip(struct in6_addr in6_addr, char *buf, int buflen)
{
	if (is_v4_mapped_ipv6(in6_addr) == 1)
	{
		unsigned long nip;
		memcpy(&nip, &in6_addr.s6_addr32[3], sizeof(long));
		inet_ntop(AF_INET, &nip, buf, buflen);
	}
	else
	{
		inet_ntop(AF_INET6, &in6_addr, buf, buflen);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	//TODO
	int server_fd = -1;
	
	if ((server_fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
	//if ((server_fd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)  //第1种设置非阻塞方法。
	{
		printf("socket() error. %d:%s\n", errno, strerror(errno)); //void perror(const char *msg);
		goto FAILED;
	}

	int flag_v6_only = 0;//0:support both ipv4 and ipv6, 1:only support ipv6, default = 0
	if (setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&flag_v6_only, sizeof(flag_v6_only)) == -1)
	{
		return	-1;
	}

	//第2种设置非阻塞方法
	set_sock_nonblock(server_fd);

	//第3中设置非阻塞方法，此方法无效
	//ioctl(server_fd, FIONBIO, 1); //写在这里提醒，与fcntl作比较。
	
	struct sockaddr_in6 server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_addr = in6addr_any;
	server_addr.sin6_port = htons(LISTEN_PORT);
	
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in6)) < 0)
	{
		printf("bind() error. %d:%s\n", errno, strerror(errno));
		goto FAILED;
	}

	if (listen(server_fd, LISTEN_QUEUE_NUM) < 0)
	{
		printf("listen() error. %d:%s\n", errno, strerror(errno));
		goto FAILED;
	}

	int client_fd = -1;
	struct sockaddr_in6 client_addr;
	socklen_t addrlen = 0;
	char client_ip[64];
	char server_ip[64];
	while (1)
	{
		do
		{
			addrlen = sizeof(client_addr);
			if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen)) < 0)
			{
				if (errno == EAGAIN)
					//printf("accept again.\n");
					;
				else
					printf("accept() error. %d:%s\n", errno, strerror(errno));
				break;
			}
			
			addrlen = sizeof(struct sockaddr_in6);
			if (getpeername(client_fd, (struct sockaddr *)&client_addr, &addrlen) < 0)
			{
				printf("getpeername() error. %d:%s\n", errno, strerror(errno));
				break;
			}

			get_str_ip(client_addr.sin6_addr, client_ip, sizeof(client_ip));
			printf("client: %s:%d\n", client_ip, ntohs(client_addr.sin6_port));
			
			addrlen = sizeof(struct sockaddr_in6);
			if (getsockname(client_fd, (struct sockaddr *)&server_addr, &addrlen) < 0)
			{
				printf("getpeername() error. %d:%s\n", errno, strerror(errno));
				break;
			}

			get_str_ip(server_addr.sin6_addr, server_ip, sizeof(server_ip));
			printf("server: %s:%d\n", server_ip, ntohs(server_addr.sin6_port));
		} while (0);
		
		if (client_fd > 0)
		{
			close(client_fd);
			client_fd = -1;
		}

		usleep(5);
	}
	

FAILED:
	if (server_fd > 0)
	{
		close(server_fd);
		server_fd = -1;
	}

	if (client_fd > 0)
	{
		close(client_fd);
		client_fd = -1;
	}
	
}

