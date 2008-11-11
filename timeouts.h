/*
 * tree management timeouts
 */

#ifndef __TIMEOUTS_H__
#define __TIMEOUTS_H__

#include "config.h"

#include <stdlib.h>

struct ev_timeout_node {
	unsigned long long int date;
	unsigned long long int mask;
	struct ev_timeout_node *parent;
	struct ev_timeout_node *go[2];
	void (*func)(struct timeval *tv, void *);
	void *arg;
};

typedef void (*ev_timeout_run)(struct timeval *tv, void *);

/* init base */
static inline void ev_timeout_init(struct ev_timeout_node *base) {
	base->mask    = 0x0ULL;
	base->parent  = NULL;
	base->go[0]   = NULL;
	base->go[1]   = NULL;
}

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
