/*
 * Copyright (c) 2008 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
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

