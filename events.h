/*
 * Copyright (c) 2008 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

/** @file */ 

#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <ebmbtree.h>

/** init select poller */
void poll_select_register(void);

/** errors codes */
typedef enum {
	/******************************
	   no errors 
	******************************/

	/** no errors */
	EV_OK = 0,

	/******************************
	   application errors 
	******************************/

	/** ev_socket_bind: port is not a number */
	EV_ERR_NOT_PORT = -0x002,

	/** address unavalaible */
	EV_ERR_NOT_ADDR = -0x003,

	/******************************
	   system error ( see errno ) 
	******************************/

	/** error with syscall socket */
	EV_ERR_SOCKET   = -0x100,

	/** error with syscall fcntl */
	EV_ERR_FCNTL    = -0x101,

	/** error with syscall setsockopt */
	EV_ERR_SETSOCKO = -0x102,

	/** error with syscall bind */
	EV_ERR_BIND     = -0x103,

	/** error with syscall listen */
	EV_ERR_LISTEN   = -0x104,

	/** error with syscall accept */
	EV_ERR_ACCEPT   = -0x105,

	/** error with syscall calloc */
	EV_ERR_CALLOC   = -0x106,

	/** error with syscall select */
	EV_ERR_SELECT   = -0x107,

	/** error with syscall sigaction */
	EV_ERR_SIGACTIO = -0x108,

	/** error with syscall sigaction */
	EV_ERR_MALLOC   = -0x109,

	/** error with syscall setsockopt */
	EV_ERR_GETSOCKO = -0x10a,

	/** error with syscall setsockopt */
	EV_ERR_CONNECT  = -0x10b

} ev_errors;

/** event mode */
typedef enum {

	/** The file descriptor operation is about read */
	EV_POLL_READ  = 0,

	/** The file descriptor operation is about write */
	EV_POLL_WRITE = 1

} ev_mode;

/** define signal event mode */
typedef enum {

	/** define the event signal synchronous */
	EV_SIGNAL_SYNCH  = 0,

	/** define the event signal asynchronous */
	EV_SIGNAL_ASYNCH = 1

} ev_synch;

struct ev_timeout_node;

/** Callback prototype for timeout events */
typedef void (*ev_timeout_run)(struct timeval *tv, struct ev_timeout_node *self, void *);

/** Callback prototype for signal event */
typedef void (*ev_signal_run)(int signal, void *arg);

/** Callback prototype for file descriptor event */
typedef void (*ev_poll_cb_wakeup)(int fd, void *arg);

/** signals event management */
struct ev_signals_register {
	unsigned int nb;
	ev_signal_run func;
	void *arg;
	ev_synch sync;
	unsigned char hide;
};

/** timeouts events management node */
struct ev_timeout_basic_node {
	struct eb_root root;
};

/** timeouts events management */
struct ev_timeout_node {
	struct {
		struct ebmb_node node;
		struct timeval tv;
	} node;
	ev_timeout_run func;
	void *arg;
};

/** poller functions pointer */
struct ev_poller {
	ev_errors (*init)(int maxfd, struct ev_timeout_basic_node *timeout_base);
	int (*fd_is_set)(int fd, ev_mode mode);
	void (*fd_set)(int fd, ev_mode mode, ev_poll_cb_wakeup func, void *arg);
	void (*fd_clr)(int fd, ev_mode mode);
	void (*fd_zero)(ev_mode mode);
	ev_errors (*poll)(void);
};

extern struct ev_poller           ev_poll;

/** Contain the current time */
extern struct timeval             ev_now;
extern struct ev_signals_register ev_signals[];


/****************************************************************************
*
* POLLER
*
****************************************************************************/

/** 
 * init events system 
 *
 * @param maxfd      contain the number of file descriptor used for the poller
 *
 * @param tmoutbase  contain the base of the timeouts tree
 *
 * @return           On success, return 0, else return error code < 0.
 *                   The error can be EV_ERR_CALLOC.
 */
static inline ev_errors ev_poll_init(int maxfd, struct ev_timeout_basic_node *tmoutbase) {
	return ev_poll.init(maxfd, tmoutbase);
}

/**
 * check if file descriptor is set 
 *
 * @param fd     is the watched filedescriptor 
 *
 * @param mode   is for choosing event register
 *
 * @return       Return true if the file descriptor is set, else return false
 */
static inline int ev_poll_fd_is_set(int fd, ev_mode mode) {
	return ev_poll.fd_is_set(fd, mode);
}

/** 
 * add event for a file descriptor 
 *
 * @param fd     is the watched filedescriptor
 *
 * @param mode   is for choosing event register
 *
 * @param func   is event function pointer
 *
 * @param arg    is easy argument gived to event function
 */
static inline void ev_poll_fd_set(int fd, ev_mode mode,
                                  ev_poll_cb_wakeup func, void *arg) {
	ev_poll.fd_set(fd, mode, func, arg);
}

/**
 * remove event for a file descriptor
 *
 * @param fd    is the removed filedescriptor
 *
 * @param mode  is for choosing event register
 */
static inline void ev_poll_fd_clr(int fd, ev_mode mode) {
	ev_poll.fd_clr(fd, mode);
}

/**
 * clear all events 
 *
 * @param mode   is for choosing event register
 */
static inline void ev_poll_fd_zero(ev_mode mode) {
	ev_poll.fd_zero(mode);
}

/**
 * run poller
 *
 * @param dont_return  if dont_return == 1, the function never return
 * @return             On success, return 0, else return error code < 0.
 *                     The error can be EV_ERR_SELECT.
 */
static inline ev_errors ev_poll_poll(int dont_return) {
	int ret;
	do {
		ret = ev_poll.poll();
	} while(dont_return);
	return ret;
}


/****************************************************************************
*
* SYSTEM SIGNALS
*
****************************************************************************/

/**
 * Init signals system
 */
void ev_signal_init(void);

/**
 * add signal 
 *
 * @param signal   is signal number (see /usr/include/bits/signum.h
 *                 on common linux systems)
 *
 * @param sync     is the synchronous mode. with EV_SIGNAL_SYNCH the
 *                 callback is called when the signal is received. With the
 *                 EV_SIGNAL_ASYNCH, the callback wait for a
 *                 ev_signal_check_active functioncall.
 *
 * @param func     is signal function pointer
 *
 * @param arg      is easy argument gived to signal function
 *
 * @return         EV_OK if ok, else < 0 if an error is occured.
 *                 The error can be EV_ERR_SIGACTIO
 */
ev_errors ev_signal_add(int signal, ev_synch sync, ev_signal_run func, void *arg);

/**
 * hide signal: the signal is ignored. If run queue contain previous signals
 * theses are deleted
 *
 * @param signal   is signal number (see /usr/include/bits/signum.h
 *                 on common linux systems)
 */
static inline void ev_signal_hide(int signal) {
	ev_signals[signal].nb   = 0;
	ev_signals[signal].hide = 1;
}

/**
 * show signal: the signal is now consider
 *
 * @param signal   is signal number (see /usr/include/bits/signum.h
 *                 on common linux systems)
 */
static inline void ev_signal_show(int signal) {
	ev_signals[signal].hide = 0;
}

/**
 * check for active signal and call callbacks
 */
void ev_signal_check_active(void);


/****************************************************************************
*
* SOCKETS
*
****************************************************************************/

/** 
 * create and connect socket
 *
 * @param socket_name  like "<ipv4>:<port>" "<ipv6>:<port>", "socket_unix_file"
 *                     or NULL. If NULL, bind is set to socket 0
 *
 * @return             if ok, return the file descriptor, else return < 0. the
 *                     errors can be: EV_ERR_NOT_PORT, EV_ERR_NOT_ADDR,
 *                     EV_ERR_SOCKET, EV_ERR_FCNTL, EV_ERR_SETSOCKO,
 *                     EV_ERR_CONNECT.
 */
ev_errors ev_socket_connect(char *socket_name);

/**
 * finish socket connexion and check result
 *
 * @param fd  the checked file descriptor
 *
 * @return    EV_OK if ok else < 0. the errors can be: EV_ERR_GETSOCKO or
 *            EV_ERR_CONNECT
 */
ev_errors ev_socket_connect_check(int fd);

/**
 * create and bind datagram socket
 *
 * @param socket_name  like "<ipv4>:<port>" "<ipv6>:<port>", "socket_unix_file"
 *                     or NULL. If NULL, bind is set to socket 0
 *
 * @param backlog      The backlog parameter defines the maximum length the queue
 *                     of pending connections may grow to. (see man listen)
 *
 * @return             if ok, return the file descriptor, else return < 0. the
 *                     errors can be: EV_ERR_NOT_PORT, EV_ERR_NOT_ADDR,
 *                     EV_ERR_SOCKET, EV_ERR_FCNTL, EV_ERR_SETSOCKO,
 *                     EV_ERR_BIND or EV_ERR_LISTEN.
 */
ev_errors ev_socket_dgram_bind(char *socket_name, int backlog);

/**
 * create and bind a socket 
 *
 * @param socket_name  like "<ipv4>:<port>" "<ipv6>:<port>", "socket_unix_file"
 *                     or NULL. If NULL, bind is set to socket 0
 *
 * @param backlog      The backlog parameter defines the maximum length the queue
 *                     of pending connections may grow to. (see man listen)
 *
 * @return             if ok, return the file descriptor, else return < 0. the
 *                     errors can be: EV_ERR_NOT_PORT, EV_ERR_NOT_ADDR,
 *                     EV_ERR_SOCKET, EV_ERR_FCNTL, EV_ERR_SETSOCKO,
 *                     EV_ERR_BIND or EV_ERR_LISTEN.
 */
ev_errors ev_socket_bind(char *socket_name, int backlog);

/**
 * create and bind a socket
 *
 * @param socket_name  like "<ipv4>:<port>" "<ipv6>:<port>", "socket_unix_file"
 *                     or NULL. If NULL, bind is set to socket 0
 *
 * @param backlog      The backlog parameter defines the maximum length the queue
 *                     of pending connections may grow to. (see man listen)
 *
 * @param protocol     the socket protocol. SOCK_STREAM or SOCK_DGRAM
 *
 * @return             if ok, return the file descriptor, else return < 0. the
 *                     errors can be: EV_ERR_NOT_PORT, EV_ERR_NOT_ADDR,
 *                     EV_ERR_SOCKET, EV_ERR_FCNTL, EV_ERR_SETSOCKO,
 *                     EV_ERR_BIND or EV_ERR_LISTEN.
 */
ev_errors ev_socket_bind_opts(char *socket_name, int backlog, int protocol);

/**
 * accept connection 
 *
 * @param listen_socket  is a socket that has been created with ev_socket_bind
 *                       and is listening for connections. (see man accept)
 *
 * @param addr           A pointer to the preallocated struct. This struct is
 *                       filled with a client address.
 *
 * @return               if ok, return new file desciptor. else return <0. the
 *                       errors can be EV_ERR_ACCEPT, EV_ERR_FCNTL or
 *                       EV_ERR_SETSOCKO.
 */
ev_errors ev_socket_accept(int listen_socket, struct sockaddr_storage *addr);


/****************************************************************************
*
* TIMEOUTS
*
****************************************************************************/

/** init timeouts tree base
 *
 * @param base  preallocated base node
 */
static inline void ev_timeout_init(struct ev_timeout_basic_node *base) {
	base->root = EB_ROOT;
}

/**
 * allocate memory for new timeout node
 *
 * @return   ptr on allocated node, NULL if error
 */
static inline
struct ev_timeout_node *ev_timeout_new(void)
{
	return calloc(1, sizeof(struct ev_timeout_node));
}

/**
 * set timeout information into node
 *
 * @param tv    date of the timeout
 *
 * @param func  callback
 *
 * @param arg   easy arg
 *
 * @param node  preallocated node
 */
static inline
void ev_timeout_build(struct timeval *tv,
                      ev_timeout_run func, void *arg,
                      struct ev_timeout_node *node)
{
	node->func     = func;
	node->arg      = arg;
	node->node.tv  = *tv;
}

/** 
 * insert timeout node into tree
 *
 * @param base   preallocated base node
 *
 * @param node   preallocated inserting 
 *
 * @return       EV_OK if ok, < 0 if an error is occured. the error code can
 *               be EV_ERR_MALLOC
 */
static inline
ev_errors ev_timeout_insert(struct ev_timeout_basic_node *base,
                            struct ev_timeout_node *node)
{
	ebmb_insert(&base->root, (struct ebmb_node *)&node->node, sizeof(struct timeval));
	return EV_OK;
}

/** 
 * free memory for node
 *
 * @param val   is a pointer to the freed node
 */
static inline
void ev_timeout_free(struct ev_timeout_node *val)
{
	free(val);
}

/**
 * remove timeout node from tree
 *
 * @param val   is a pointer to the freed node
 */
static inline
void ev_timeout_remove(struct ev_timeout_node *val)
{
	ebmb_delete((struct ebmb_node *)&val->node);
}

/** 
 * get min time 
 *
 * @param base   preallocated base node
 *
 * @return       return a pointer to the min timeout node
 */
static inline
struct ev_timeout_node *ev_timeout_get_min(struct ev_timeout_basic_node *base)
{
	struct ebmb_node *node;
	node = ebmb_first(&base->root);
	return ebmb_entry(node, struct ev_timeout_node, node.node);
}

/**
 * get minx time
 *
 * @param base   preallocated base node
 *
 * @return       return a pointer to the max timeout node
 */
static inline
struct ev_timeout_node *ev_timeout_get_max(struct ev_timeout_basic_node *base)
{
	struct ebmb_node *node;
	node = ebmb_last(&base->root);
	return ebmb_entry(node, struct ev_timeout_node, node.node);
}

/**
 * get next node
 *
 * @param current   preallocated base node
 *
 * @return          return a pointer to the next timeout node
 */
static inline
struct ev_timeout_node *ev_timeout_get_next(struct ev_timeout_node *current)
{
	struct ebmb_node *node;
	node = ebmb_next((struct ebmb_node *)&current->node);
	return ebmb_entry(node, struct ev_timeout_node, node.node);
}

/**
 * get prev node
 *
 * @param current   preallocated base node
 *
 * @return          return a pointer to the prev timeout node
 */
static inline
struct ev_timeout_node *ev_timeout_get_prev(struct ev_timeout_node *current)
{
	struct ebmb_node *node;
	node = ebmb_prev((struct ebmb_node *)&current->node);
	return ebmb_entry(node, struct ev_timeout_node, node.node);
}

/**
 * check if the time exist 
 *
 * @param base   preallocated base node
 *
 * @param tv     time
 *
 * @return       return a pointer to the prev timeout node
 *               or NULL if dont exists time
 */
static inline
struct ev_timeout_node *ev_timeout_exists(struct ev_timeout_basic_node *base,
                                          struct timeval *tv)
{
	struct ebmb_node *node;
	node = ebmb_lookup(&base->root, tv, sizeof(struct timeval));
	return node == NULL ? NULL : ebmb_entry(node, struct ev_timeout_node, node.node);
}

/**
 * extract timeval
 *
 * @param val    preallocated base node
 *
 * @param tv     the timeout date is stored here
 */
static inline
void ev_timeout_get_tv(struct ev_timeout_node *val,
                       struct timeval *tv)
{
	*tv = val->node.tv;
}

/**
 * extract function
 *
 * @param val    preallocated base node
 *
 * @return       a pointer to the callback
 */
static inline
ev_timeout_run ev_timeout_get_func(struct ev_timeout_node *val)
{
	return val->func;
}

/**
 * extract value
 *
 * @param val    preallocated base node
 *
 * @return       the easy argument
 */
static inline
void *ev_timeout_get_arg(struct ev_timeout_node *val)
{
	return val->arg;
}

/**
 * set timeout information into node
 *
 * @param node  preallocated node
 *
 * @param tv    date of the timeout
 */
static inline
void ev_timeout_set_tv(struct ev_timeout_node *val,
                       struct timeval *tv)
{
	val->node.tv = *tv;
}

/**
 * set function
 *
 * @param val    preallocated base node
 *
 * @return       a pointer to the callback
 */
static inline void ev_timeout_set_func(struct ev_timeout_node *val,
                                       ev_timeout_run func) {
	val->func = func;
}

/**
 * set value
 *
 * @param val    preallocated base node
 *
 * @return       the easy argument
 */
static inline void ev_timeout_set_arg(struct ev_timeout_node *val,
                                      void *arg) {
	val->arg = arg;
}

/**
 * call function
 *
 * @param val    preallocated base node
 */
static inline
void ev_timeout_call_func(struct ev_timeout_node *val)
{
	struct timeval tv;
	ev_timeout_get_tv(val, &tv);
	val->func(&tv, val, val->arg);
}

#endif
