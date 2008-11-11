#define __POLL_C__

#include <time.h>
#include <sys/time.h>

#include "poll.h"

struct poller poll;
struct timeval now;

