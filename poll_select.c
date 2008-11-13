/*
 * Copyright (c) 2008 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>

#include "events.h"

struct rfds_cb {
	ev_poll_cb_wakeup func;
	void *arg;
};

static fd_set *rfds[2];
static fd_set *polled_rfds[2];
static int nb_fd_set;
static int maxconn;
static struct rfds_cb *callback_rfds[2];
static struct ev_timeout_node *tmoutbase;

static int poll_select_fd_is_set(int fd, ev_mode mode) {
	return FD_ISSET(fd, rfds[mode]);
}

static void poll_select_fd_set(int fd, ev_mode mode,
                               ev_poll_cb_wakeup func, void *arg) {
	FD_SET(fd, rfds[mode]);
	callback_rfds[mode][fd].func = func;
	callback_rfds[mode][fd].arg  = arg;
}

static void poll_select_fd_clr(int fd, ev_mode mode) {
	FD_CLR(fd, rfds[mode]);
	callback_rfds[mode][fd].func = NULL;
	callback_rfds[mode][fd].arg  = NULL;
}

static void poll_select_fd_zero(ev_mode mode) {
	int i;

	FD_ZERO(rfds[mode]);
	for (i=0; i<maxconn; i++) {
		callback_rfds[mode][i].func = NULL;
		callback_rfds[mode][i].arg  = NULL;
	}
}

static ev_errors poll_select_poll(void) {
	int ret_code;
	int i;
	unsigned int cp_read;
	unsigned int cp_write;
	int maxfd;
	struct timeval tv, diff;
	struct timeval *tmout;
	fd_set *fd_read;
	fd_set *fd_write;
	struct ev_timeout_node *t;

	// timeouts
	tmout = NULL;
	while (1) {
		t = ev_timeout_get_min(tmoutbase);

		// pas de timeout : on sort
		if (t == NULL)
			break;

		ev_timeout_get_tv(t, &tv);
		diff.tv_sec  = tv.tv_sec  - ev_now.tv_sec;
		diff.tv_usec = tv.tv_usec - ev_now.tv_usec;
		if (diff.tv_usec < 0) {
			diff.tv_usec += 1000000;
			diff.tv_sec  -= 1;
		}

		// execution des timeouts si besoin
		if (diff.tv_sec < 0 || ( diff.tv_sec == 0 && diff.tv_usec < 0) )
			ev_timeout_call_func(t);

		// sinon, on sort
		else {
			tmout = &diff;
			break;
		}
	}


	// calcule le maxfd
	// et copie les blocs de poll
	maxfd = 0;
	fd_read = NULL;
	fd_write = NULL;
	for (i = ( (nb_fd_set * FD_SETSIZE) / (sizeof(unsigned int)*8) ) - 1;
	     i>=0;
	     i--) {
		cp_read = *((unsigned int *)rfds[EV_POLL_READ]+i);
		cp_write = *((unsigned int *)rfds[EV_POLL_WRITE]+i);
		*((unsigned int *)polled_rfds[EV_POLL_READ]+i) = cp_read;
		*((unsigned int *)polled_rfds[EV_POLL_WRITE]+i) = cp_write;

		if (cp_read != 0) {
			fd_read = polled_rfds[EV_POLL_READ];
			for (maxfd=((i+1)*sizeof(unsigned int)*8)-1;
			     maxfd<i*sizeof(unsigned int)*8;
			     maxfd--)
				if (FD_ISSET(maxfd, rfds[EV_POLL_READ]))
					break;
			break;
		}

		if (cp_write != 0) {
			fd_write = polled_rfds[EV_POLL_WRITE];
			for (maxfd=((i+1)*sizeof(unsigned int)*8)-1;
			     maxfd<i*sizeof(unsigned int)*8;
			     maxfd--)
				if (FD_ISSET(maxfd, rfds[EV_POLL_WRITE]))
					break;
			break;
		}
	}

	// fin de la copie
	for (; i>=0; i--) {
		cp_read = *((unsigned int *)rfds[EV_POLL_READ]+i);
		cp_write = *((unsigned int *)rfds[EV_POLL_WRITE]+i);
		*((unsigned int *)polled_rfds[EV_POLL_READ]+i) = cp_read;
		*((unsigned int *)polled_rfds[EV_POLL_WRITE]+i) = cp_write;

		if (cp_read != 0)
			fd_read = polled_rfds[EV_POLL_READ];

		if (cp_write != 0)
			fd_write = polled_rfds[EV_POLL_WRITE];
	}

	// au lit ...
	ret_code = select(maxfd+1, fd_read, fd_write, NULL, tmout);

	// on mets l'heure a jour
	gettimeofday(&ev_now, NULL);

	// check signals
	ev_signal_check_active();

	// erreurs
	if (ret_code < 0)
		return EV_ERR_SELECT;
	
	// timeouts
	else if (ret_code == 0)
		ev_timeout_call_func(t);

	// on analyse le retour
	else

		for (i = 0;
		     i < ((maxfd-1)/(sizeof(unsigned int)*8))+1;
		     i++) {

			if (*((unsigned int *)polled_rfds[EV_POLL_READ]+i) != 0)
				for (maxfd = i * sizeof(unsigned int) * 8;
				     maxfd < ( i + 1 ) * sizeof(unsigned int) * 8;
				     maxfd ++) {
					if (FD_ISSET(maxfd, polled_rfds[EV_POLL_READ])) {
						if (callback_rfds[EV_POLL_READ][maxfd].func != NULL)
							callback_rfds[EV_POLL_READ][maxfd].func(maxfd, 
							                 callback_rfds[EV_POLL_READ][maxfd].arg);
					}
				}

			if (*((unsigned int *)polled_rfds[EV_POLL_WRITE]+i) != 0)
				for (maxfd = i * sizeof(unsigned int) * 8;
				     maxfd < ( i + 1 ) * sizeof(unsigned int) * 8;
				     maxfd ++) {
					if (FD_ISSET(maxfd, polled_rfds[EV_POLL_WRITE])) {
						if (callback_rfds[EV_POLL_WRITE][maxfd].func != NULL)
							callback_rfds[EV_POLL_WRITE][maxfd].func(maxfd,
							                 callback_rfds[EV_POLL_READ][maxfd].arg);
					}
				}
	
		}

	return 0;
}

static ev_errors poll_select_init(int maxfd, struct ev_timeout_node *timeout_base) {
	maxconn = maxfd;
	tmoutbase = timeout_base;

	// maxconn sera toujours au moins egal a 1 et donc
	// jamais negatif
	nb_fd_set = ( ( maxconn - 1 ) / FD_SETSIZE ) + 1; 

	rfds[EV_POLL_READ] = (fd_set *)calloc(sizeof(fd_set), nb_fd_set);
	if (rfds[EV_POLL_READ] == NULL)
		return EV_ERR_CALLOC;

	rfds[EV_POLL_WRITE] = (fd_set *)calloc(sizeof(fd_set), nb_fd_set);
	if (rfds[EV_POLL_WRITE] == NULL)
		return EV_ERR_CALLOC;

	polled_rfds[EV_POLL_READ] = (fd_set *)calloc(sizeof(fd_set), nb_fd_set);
	if (polled_rfds[EV_POLL_READ] == NULL)
		return EV_ERR_CALLOC;

	polled_rfds[EV_POLL_WRITE] = (fd_set *)calloc(sizeof(fd_set), nb_fd_set);
	if (polled_rfds[EV_POLL_WRITE] == NULL)
		return EV_ERR_CALLOC;

	callback_rfds[EV_POLL_READ] = (struct rfds_cb *)calloc(
	                           sizeof(struct rfds_cb), maxconn);
	if (callback_rfds[EV_POLL_READ] == NULL)
		return EV_ERR_CALLOC;

	callback_rfds[EV_POLL_WRITE] = (struct rfds_cb *)calloc(
	                            sizeof(struct rfds_cb), maxconn);
	if (callback_rfds[EV_POLL_WRITE] == NULL)
		return EV_ERR_CALLOC;

	// on mets l'heure a jour
	gettimeofday(&ev_now, NULL);

	return 0;
}

// a degager plus tard
/*
__attribute__((constructor))
static void poll_select_register(void) {
*/
void poll_select_register(void) {
	ev_poll.init        = poll_select_init;
	ev_poll.fd_is_set   = poll_select_fd_is_set;
	ev_poll.fd_set      = poll_select_fd_set;
	ev_poll.fd_clr      = poll_select_fd_clr;
	ev_poll.fd_zero     = poll_select_fd_zero;
	ev_poll.poll        = poll_select_poll;
}

