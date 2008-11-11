/*
 * Copyright (c) 2008 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

#include "config.h"

#include <signal.h>
//#include <stdlib.h>
#include <string.h>
//#include <errno.h>

#include "signals.h"

#define NB_SIG _NSIG

struct ev_signals_register signals[NB_SIG];

int nbsigs = 0;

/* function called for each signal */
static void ev_signal_interrupt(int signal) {
	if (signals[signal].func == NULL ||
	    signals[signal].hide == 1)
		return;
	
	// increment counter for later run
	if (signals[signal].sync == EV_SIGNAL_SYNCH) {
		nbsigs++;
		signals[signal].nb++;
	}

	// run function
	else 
		signals[signal].func(signal, signals[signal].arg);
}

/* init signal */
int ev_signal_add(int signal, int sync, ev_signal_run func, void *arg) {
	struct sigaction old, new;

	memset(&new, 0, sizeof(struct sigaction));
	new.sa_handler = ev_signal_interrupt;
	new.sa_flags = SA_RESTART;
	sigemptyset(&(new.sa_mask));
	if (sigaction(signal, &new, &old))
		return 1;
	
	signals[signal].nb   = 0;
	signals[signal].hide = 0;
	signals[signal].sync = sync;
	signals[signal].func = func;
	signals[signal].arg  = arg;

	return 0;
}

/* check for active signal */
void ev_signal_check_active(void) {
	int i, j;

	if (nbsigs == 0)
		return;

	for (i=0; i<NB_SIG; i++) {
		if (signals[i].func == NULL)
			continue;

		for(j = signals[i].nb; j>0; j++) {
			signals[i].func(i, signals[i].arg);
		}
	}
}

/* signal initialization */
__attribute__((constructor))
static void ev_signal_init(void) {
	int i;

	for (i=0; i<NB_SIG; i++) {
		signals[i].func = NULL;
	}
}

