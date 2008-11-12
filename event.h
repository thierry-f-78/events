/*
 * Copyright (c) 2008 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>

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

typedef void (*ev_timeout_run)(struct timeval *tv, void *);
typedef void (*ev_signal_run)(int signal, void *arg);
typedef void (*ev_poll_cb_wakeup)(int fd, void *arg);

struct ev_signals_register {
	unsigned int nb;
	ev_signal_run func;
	void *arg;
	unsigned char hide;
	unsigned char sync;
};

struct ev_timeout_node {
	unsigned long long int date;
	unsigned long long int mask;
	struct ev_timeout_node *parent;
	struct ev_timeout_node *go[2];
	void (*func)(struct timeval *tv, void *);
	void *arg;
};

struct poller {
	int (*init)(int maxfd, struct ev_timeout_node *timeout_base);
	int (*fd_is_set)(int fd, int mode);
	void (*fd_set)(int fd, int mode, ev_poll_cb_wakeup func, void *arg);
	void (*fd_clr)(int fd, int mode);
	void (*fd_zero)(int mode);
	int (*poll)(void);
};

extern struct poller poll;
extern struct timeval now;
extern struct ev_signals_register signals[];

/****************************************************************************
*
* POLLER
*
****************************************************************************/

#define EV_POLL_READ  0
#define EV_POLL_WRITE 1

/* init events system */
static inline int ev_poll_init(int maxfd, struct ev_timeout_node *tmoutbase) {
	return poll.init(maxfd, tmoutbase);
}

/* check if file descriptor is set */
static inline int ev_poll_fd_is_set(int fd, int mode) {
	return poll.fd_is_set(fd, mode);
}

/* add event for a file descriptor */
static inline void ev_poll_fd_set(int fd, int mode,
                                  ev_poll_cb_wakeup func, void *arg) {
	poll.fd_set(fd, mode, func, arg);
}

/* remove event for a file descriptor */
static inline void ev_poll_fd_clr(int fd, int mode) {
	poll.fd_clr(fd, mode);
}

/* clear all events */
static inline void ev_poll_fd_zero(int mode) {
	poll.fd_zero(mode);
}

/* run poller */
static inline int ev_poll_poll(void) {
	return poll.poll();
}

/****************************************************************************
*
* SYSTEM SIGNALS
*
****************************************************************************/

#define EV_SIGNAL_SYNCH  0
#define EV_SIGNAL_ASYNCH 1

/* add signal */
int ev_signal_add(int signal, int sync, ev_signal_run func, void *arg);

/* hide signal */
static inline void ev_signal_hide(int signal) {
	signals[signal].nb   = 0;
	signals[signal].hide = 1;
}

/* show signal */
static inline void ev_signal_show(int signal) {
	signals[signal].hide = 0;
}

/* check for active signal */
void ev_signal_check_active(void);

/****************************************************************************
*
* SOCKETS
*
****************************************************************************/

/* create and bind a socket */
int ev_socket_bind(char *socket_name, int maxconn);

/* accept connection */
int ev_socket_accept(int listen_socket, struct sockaddr_storage *addr);

/* init base */
static inline void ev_timeout_init(struct ev_timeout_node *base) {
	base->mask    = 0x0ULL;
	base->parent  = NULL;
	base->go[0]   = NULL;
	base->go[1]   = NULL;
}

/****************************************************************************
*
* TIMEOUTS
*
****************************************************************************/

/* insert */
struct ev_timeout_node *ev_timeout_add(struct ev_timeout_node *base,
                                       struct timeval *tv, ev_timeout_run func,
                                       void *arg);

/* delete */
void ev_timeout_del(struct ev_timeout_node *val);

/* get min time */
struct ev_timeout_node *ev_timeout_get_min(struct ev_timeout_node *base);

/* get minx time */
struct ev_timeout_node *ev_timeout_get_max(struct ev_timeout_node *base);

/* get next node */
struct ev_timeout_node *ev_timeout_get_next(struct ev_timeout_node *current);

/* get prev node */
struct ev_timeout_node *ev_timeout_get_prev(struct ev_timeout_node *current);

/* check if the time exist */
struct ev_timeout_node *ev_timeout_exists(struct ev_timeout_node *base,
                                          struct timeval *tv);

/* extract timeval */
static inline void ev_timeout_get_tv(struct ev_timeout_node *val,
                                     struct timeval *tv) {
	tv->tv_sec  = val->date >> 32;
	tv->tv_usec = val->date & 0x00000000fffffffffULL;
}

/* extract function */
static inline ev_timeout_run ev_timeout_get_func(struct ev_timeout_node *val) {
	return val->func;
}

/* extract value */
static inline void *ev_timeout_get_arg(struct ev_timeout_node *val) {
	return val->arg;
}

/* call function */
static inline void ev_timeout_call_func(struct ev_timeout_node *val) {
	struct timeval tv;

	ev_timeout_get_tv(val, &tv);
	val->func(&tv, val->arg);
}

#endif
