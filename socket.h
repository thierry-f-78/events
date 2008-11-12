#ifndef __BIND_H__
#define __BIND_H__

#include <sys/socket.h>

/* errors */
#define EV_ERR_NOT_PORT -0x002 /* ev_socket_bind: port is not a number*/
#define EV_ERR_NOT_ADDR -0x003 /* address unavalaible */

/* system error ( see errno ) */
#define EV_ERR_SOCKET   -0x100 /* error with syscall socket */
#define EV_ERR_FCNTL    -0x101 /* error with syscall fcntl */
#define EV_ERR_SETSOCKO -0x102 /* error with syscall setsockopt */
#define EV_ERR_BIND     -0x103 /* error with syscall bind */
#define EV_ERR_LISTEN   -0x104 /* error with syscall listen */
#define EV_ERR_ACCEPT   -0x105 /* error with syscall accept */
#define EV_ERR_CALLOC   -0x106 /* error with syscall calloc */
#define EV_ERR_SELECT   -0x107 /* error with syscall select */

/* create and bind a socket */
int ev_socket_bind(char *socket_name, int maxconn);

/* accept connection */
int ev_socket_accept(int listen_socket, struct sockaddr_storage *addr);

#endif
