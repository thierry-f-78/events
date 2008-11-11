#ifndef __POLL_H__
#define __POLL_H__

#include <time.h>
#include <sys/time.h>

#define POLL_READ 0
#define POLL_WRITE 1

struct poller {
	void (*init)(void);
	int (*fd_is_set)(int fd, int mode);
	void (*fd_set)(int fd, int mode, void(*cb_rfds)(int fd));
	void (*fd_clr)(int fd, int mode);
	void (*fd_zero)(int mode);
	void (*set_timeout)(struct timeval timeout, void (*cb_timeout)(void));
	void (*poll)(void);
};

#ifndef __POLL_C__
extern struct poller poll;
extern struct timeval now;
#endif

#endif
