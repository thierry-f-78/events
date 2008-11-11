#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>

#include "poll.h"

static fd_set *rfds[2];
static fd_set *polled_rfds[2];
static int nb_fd_set;
static int maxconn;
static struct timeval _timeout;
static struct timeval *timeout;
static void (*callback_timeout)(void);
static void (**callback_rfds[2])(int fd);

static int poll_select_fd_is_set(int fd, int mode) {
	return FD_ISSET(fd, rfds[mode]);
}

static void poll_select_fd_set(int fd, int mode,
                               void (*cb_rfds)(int cb_fd)) {
	FD_SET(fd, rfds[mode]);
	callback_rfds[mode][fd] = cb_rfds;
}

static void poll_select_fd_clr(int fd, int mode) {
	FD_CLR(fd, rfds[mode]);
	callback_rfds[mode][fd] = NULL;
}

static void poll_select_fd_zero(int mode) {
	int i;

	FD_ZERO(rfds[mode]);
	for (i=0; i<maxconn; i++)
		callback_rfds[mode][i] = NULL;
}

static void poll_select_poll(void) {
	int ret_code;
	int i;
	unsigned int cp_read;
	unsigned int cp_write;
	int maxfd;
	fd_set *fd_read;
	fd_set *fd_write;

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
	for (;
	     i>=0;
	     i--) {
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
	ret_code = select(maxfd+1, fd_read, fd_write, NULL, timeout);

	// on mets l'heure a jour
	gettimeofday(&now, NULL);

	// on analyse le retour
	for (i = 0;
	     i < ((maxfd-1)/(sizeof(unsigned int)*8))+1;
	     i++) {
		if (*((unsigned int *)polled_rfds[EV_POLL_READ]+i) != 0) {
			for (maxfd = i * sizeof(unsigned int) * 8;
			     maxfd < ( i + 1 ) * sizeof(unsigned int) * 8;
			     maxfd ++) {
				if (FD_ISSET(maxfd, polled_rfds[EV_POLL_READ])) {
					if (callback_rfds[EV_POLL_READ][maxfd] != NULL)
						callback_rfds[EV_POLL_READ][maxfd](maxfd);
				}
			}
		}
		if (*((unsigned int *)polled_rfds[EV_POLL_WRITE]+i) != 0) {
			for (maxfd = i * sizeof(unsigned int) * 8;
			     maxfd < ( i + 1 ) * sizeof(unsigned int) * 8;
			     maxfd ++) {
				if (FD_ISSET(maxfd, polled_rfds[EV_POLL_WRITE])) {
					if (callback_rfds[EV_POLL_WRITE][maxfd] != NULL)
						callback_rfds[EV_POLL_WRITE][maxfd](maxfd);
				}
			}
		}
	}

	// reset timeout;
	timeout = NULL;
	callback_timeout = NULL;
}

static void poll_select_init(int maxfd) {
	maxconn = maxfd;

	// maxconn sera toujours au moins egal a 1 et donc
	// jamais negatif
	nb_fd_set = ( ( maxconn - 1 ) / FD_SETSIZE ) + 1; 

	rfds[EV_POLL_READ] = (fd_set *)calloc(sizeof(fd_set), nb_fd_set);
	if (rfds[EV_POLL_READ] == NULL) {
		fprintf(stderr, "calloc(%d, %d)[%d]: %s", sizeof(fd_set),
		       nb_fd_set, errno, strerror(errno));
		exit(1);
	}

	rfds[EV_POLL_WRITE] = (fd_set *)calloc(sizeof(fd_set), nb_fd_set);
	if (rfds[EV_POLL_WRITE] == NULL) {
		fprintf(stderr, "calloc(%d, %d)[%d]: %s", sizeof(fd_set),
		       nb_fd_set, errno, strerror(errno));
		exit(1);
	}

	polled_rfds[EV_POLL_READ] = (fd_set *)calloc(sizeof(fd_set), nb_fd_set);
	if (polled_rfds[EV_POLL_READ] == NULL) {
		fprintf(stderr, "calloc(%d, %d)[%d]: %s", sizeof(fd_set),
		       nb_fd_set, errno, strerror(errno));
		exit(1);
	}

	polled_rfds[EV_POLL_WRITE] = (fd_set *)calloc(sizeof(fd_set), nb_fd_set);
	if (polled_rfds[EV_POLL_WRITE] == NULL) {
		fprintf(stderr, "calloc(%d, %d)[%d]: %s", sizeof(fd_set),
		       nb_fd_set, errno, strerror(errno));
		exit(1);
	}

	callback_rfds[EV_POLL_READ] = (void (**)(int fd))calloc(
	                           sizeof(void (*)(int fd)), maxconn);
	if (callback_rfds[EV_POLL_READ] == NULL) {
		fprintf(stderr, "calloc(%d, %d)[%d]: %s",
		       sizeof(fd_set), nb_fd_set, errno, strerror(errno));
		exit(1);
	}

	callback_rfds[EV_POLL_WRITE] = (void (**)(int fd))calloc(
	                            sizeof(void (*)(int fd)), maxconn);
	if (callback_rfds[EV_POLL_WRITE] == NULL) {
		fprintf(stderr, "calloc(%d, %d)[%d]: %s",
		       sizeof(fd_set), nb_fd_set, errno, strerror(errno));
		exit(1);
	}
}

// a degager plus tard
__attribute__((constructor))
static void poll_select_register(void) {
	poll.init        = poll_select_init;
	poll.fd_is_set   = poll_select_fd_is_set;
	poll.fd_set      = poll_select_fd_set;
	poll.fd_clr      = poll_select_fd_clr;
	poll.fd_zero     = poll_select_fd_zero;
	poll.poll        = poll_select_poll;
}

