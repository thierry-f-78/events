/*
 * Copyright (c) 2005-2010 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * $Id: arpalert.c 421 2006-11-04 10:56:25Z thierry $
 *
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "log.h"

// set flag signals
int sigchld = 0;
int sighup  = 0;
int sigstop = 0;

void setsigchld(int signal){
	sigchld++;
	logmsg(LOG_DEBUG, "SIGCHLD");
}
void setsighup(int signal){
	sighup++;
	logmsg(LOG_DEBUG, "SIGHUP");
}
void setsigstop(int signal){
	sigstop++;
	logmsg(LOG_DEBUG, "SIGSTOP");
	exit(0);
}
void setsigpipe(int signal){
}

// setup signals
void (*setsignal (int signal, void (*function)(int)))(int) {
	struct sigaction old, new;

	memset(&new, 0, sizeof(struct sigaction));
	new.sa_handler = function;
	new.sa_flags = SA_RESTART;
	sigemptyset(&(new.sa_mask));
	if (sigaction(signal, &new, &old)) { 
		logmsg(LOG_CRIT, "sigaction[%d]: %s", errno, strerror(errno));
		exit(1);
	}
	return(old.sa_handler);
}

// init signals
void signals_init(void){
	(void)setsignal(SIGINT,  setsigstop);
	(void)setsignal(SIGTERM, setsigstop);
	(void)setsignal(SIGQUIT, setsigstop);
	(void)setsignal(SIGABRT, setsigstop);
	(void)setsignal(SIGCHLD, setsigchld); 
	(void)setsignal(SIGHUP,  setsighup); 
	(void)setsignal(SIGPIPE, setsigpipe); 
}

// signals computing
void signals_func(void){

	// SIGCHLD
	if(sigchld > 0){
		sigchld--;
	}
	// SIGHUP
	if(sighup > 0){
		sighup--;
	}
	// SIGQUIT
	if(sigstop > 0){
		sigstop--;
	}
}

