#ifndef __BIND_H__
#define __BIND_H__

#include <sys/socket.h>

int bind_init(char *socket_name, int maxconn);
int bind_accept(int listen_socket, struct sockaddr_storage *addr);

#endif
