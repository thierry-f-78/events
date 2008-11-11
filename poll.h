#ifndef __POLL_H__
#define __POLL_H__

#include <time.h>
#include <sys/time.h>

#define EV_POLL_READ  0
#define EV_POLL_WRITE 1

typedef void(*ev_poll_cb_wakeup)(int fd, void *arg);
typedef void(*ev_poll_cb_timeout)(int fd, void *arg);

struct poller {
	void (*init)(int maxfd);
	int (*fd_is_set)(int fd, int mode);
	void (*fd_set)(int fd, int mode, ev_poll_cb_wakeup func, void *arg);
	void (*fd_clr)(int fd, int mode);
	void (*fd_zero)(int mode);
	void (*poll)(void);
};

extern struct poller poll;
extern struct timeval now;

/* init events system */
static inline void ev_poll_init(maxfd) {
	poll.init(maxfd);
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
static inline void ev_poll_poll(void) {
	poll.poll();
}

#endif
