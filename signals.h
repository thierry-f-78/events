/*
 * Copyright (c) 2005-2010 Thierry FOURNIER
 * $Id: arpalert.c 421 2006-11-04 10:56:25Z thierry $
 *
 */

#ifndef __SIGNALS_H__
#define __SIGNALS_H__

#include <time.h>
#include <sys/time.h>

#define EV_SIGNAL_SYNCH  0
#define EV_SIGNAL_ASYNCH 1

typedef void (*ev_signal_run)(int signal, void *arg);

struct ev_signals_register {
	unsigned int nb;
	ev_signal_run func;
	void *arg;
	unsigned char hide;
	unsigned char sync;
};

extern struct ev_signals_register signals[];

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

#endif

