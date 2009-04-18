/*
 * Copyright (c) 2008 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

#include <signal.h>
#include <string.h>

#include "events.h"

#define NB_SIG _NSIG

struct ev_signals_register ev_signals[NB_SIG];

int nbsigs = 0;

/* function called for each signal */
static void ev_signal_interrupt(int signal) {
	if (ev_signals[signal].func == NULL ||
	    ev_signals[signal].hide == 1)
		return;
	
	switch (ev_signals[signal].sync) {

	// increment counter for later run
	case EV_SIGNAL_ASYNCH:
		nbsigs++;
		ev_signals[signal].nb++;
		break;

	// run function
	case EV_SIGNAL_SYNCH:
		ev_signals[signal].func(signal, ev_signals[signal].arg);
		break;

	}
}

/* init signal */
ev_errors ev_signal_add(int signal, ev_synch sync, ev_signal_run func, void *arg) {
	struct sigaction old, new;

	memset(&new, 0, sizeof(struct sigaction));
	new.sa_handler = ev_signal_interrupt;
	new.sa_flags = SA_RESTART;
	sigemptyset(&(new.sa_mask));
	if (sigaction(signal, &new, &old))
		return EV_ERR_SIGACTIO;
	
	ev_signals[signal].nb   = 0;
	ev_signals[signal].hide = 0;
	ev_signals[signal].sync = sync;
	ev_signals[signal].func = func;
	ev_signals[signal].arg  = arg;

	return EV_OK;
}

/* check for active signal */
void ev_signal_check_active(void) {
	int i, j;

	if (nbsigs == 0)
		return;

	for (i=0; i<NB_SIG; i++) {
		if (ev_signals[i].func == NULL)
			continue;

		for(j = ev_signals[i].nb; j>0; j--) {
			ev_signals[i].func(i, ev_signals[i].arg);
		}
		nbsigs -= ev_signals[i].nb;
		ev_signals[i].nb = 0;
	}
}

/* signal initialization */
__attribute__((constructor))
void ev_signal_init(void) {
	int i;

	for (i=0; i<NB_SIG; i++)
		ev_signals[i].func = NULL;
}

