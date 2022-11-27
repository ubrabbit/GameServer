#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int errexit(const char *format, ...){
	va_list args;

	va_start(args,format);
	vfprintf(stderr, format, args);
	va_end(args);

	exit(1);
}

int connect_remote(const char *host, int16_t port, const char *proname){
	struct hostent*phe;
	struct servent *pse;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

	if(NULL != (pse = getservbyname(host, proname)))
	{
		sin.sin_port=pse->s_port;
	}
	else if((sin.sin_port = htons((unsigned short)port))==0)
	{
		errexit("can't get \"%s\" port entry\n", port);
	}

	if(NULL != (phe = gethostbyname(host)))
	{
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	else if((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
	{
		errexit("can't get \"%s\" prptocol entry\n", proname);
	}

	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		errexit("can't create socket: %s\n", strerror(errno));
	}

	if(connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		errexit("can't connect to %s.%s: %s\n", host, port, strerror(errno));
	}

	return sockfd;
}
