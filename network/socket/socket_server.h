/*
The MIT License (MIT)

Copyright (c) 2013 codingnow.com Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://github.com/cloudwu/skynet
*/

#ifndef skynet_socket_server_h
#define skynet_socket_server_h

#include <stdint.h>

// custom malloc/free
//#define SOCKET_SERVER_FILE_MEMAPI   skynet.h
//#define SOCKET_SERVER_MALLOC        skynet_malloc
//#define SOCKET_SERVER_FREE          skynet_free

#define SOCKET_DATA 0
#define SOCKET_CLOSE 1
#define SOCKET_OPEN 2
#define SOCKET_ACCEPT 3
#define SOCKET_ERROR 4
#define SOCKET_EXIT 5
#define SOCKET_UDP 6

struct socket_server;

struct socket_message {
	int id;
	uintptr_t opaque;
	int ud;	// for accept, ud is listen id ; for data, ud is size of data
	char * data;
};

struct socket_server * socket_server_create();
void socket_server_release(struct socket_server *);
int socket_server_poll(struct socket_server *, struct socket_message *result, int *more);

void socket_server_exit(struct socket_server *);
void socket_server_close(struct socket_server *, uintptr_t opaque, int id);
void socket_server_start(struct socket_server *, uintptr_t opaque, int id);

// return -1 when error
int64_t socket_server_send(struct socket_server *, int id, const void * buffer, int sz);
void socket_server_send_lowpriority(struct socket_server *, int id, const void * buffer, int sz);

// ctrl command below returns id
int socket_server_listen(struct socket_server *, uintptr_t opaque, const char * addr, int port, int backlog);
int socket_server_connect(struct socket_server *, uintptr_t opaque, const char * addr, int port);
int socket_server_bind(struct socket_server *, uintptr_t opaque, int fd);

// for tcp
void socket_server_nodelay(struct socket_server *, int id);

struct socket_udp_address;

// create an udp socket handle, attach opaque with it . udp socket don't need call socket_server_start to recv message
// if port != 0, bind the socket . if addr == NULL, bind ipv4 0.0.0.0 . If you want to use ipv6, addr can be "::" and port 0.
int socket_server_udp(struct socket_server *, uintptr_t opaque, const char * addr, int port);
// set default dest address, return 0 when success
int socket_server_udp_connect(struct socket_server *, int id, const char * addr, int port);
// If the socket_udp_address is NULL, use last call socket_server_udp_connect address instead
// You can also use socket_server_send
int64_t socket_server_udp_send(struct socket_server *, int id, const struct socket_udp_address *, const void *buffer, int sz);
// extract the address of the message, struct socket_message * should be SOCKET_UDP
const struct socket_udp_address * socket_server_udp_address(struct socket_server *, struct socket_message *, int *addrsz);

struct socket_object_interface {
	void * (*buffer)(void *);
	int (*size)(void *);
	void (*free)(void *);
};

// if you send package sz == -1, use soi.
void socket_server_userobject(struct socket_server *, struct socket_object_interface *soi);

#endif
