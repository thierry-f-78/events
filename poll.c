/*
 * Copyright (c) 2008-2012 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

#include <time.h>
#include <sys/time.h>

#include "events.h"

struct ev_poller ev_poll;
struct timeval ev_now;

