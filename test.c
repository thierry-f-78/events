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

#include "timeouts.h"

int main(void) {
	struct timeval tv;
	void *v;
	struct ev_timeout_node node;
	struct ev_timeout_node *n;

	// init
	ev_timeout_init(&node);

	tv.tv_sec=0;
	for (tv.tv_usec=0; tv.tv_usec<1000000; tv.tv_usec++)
		ev_timeout_add(&node, &tv, NULL, NULL);

	// plus petit
	/*
	n = ev_timeout_get_max)(&node);
	while (n != NULL) {
		ev_timeout_get_tv)(n, &tv);
		ev_timeout_del)(n);
		printf("min=%u.%u\n", tv.tv_sec, tv.tv_usec);
		n = ev_timeout_get_max)(&node);
	}
	*/

	/*
	n = ev_timeout_get_max)(&node);
	while (n != NULL) {
		ev_timeout_get_tv)(n, &tv);
		printf("min=%u.%u\n", tv.tv_sec, tv.tv_usec);
		n = ev_timeout_get_prev)(n);
	}
	*/

	printf("eoi\n");
	tv.tv_sec=0;
	tv.tv_usec=999999;
	if (ev_timeout_exists(&node, &tv) != NULL)
		printf("%u.%u exist\n", tv.tv_sec, tv.tv_usec);
	else
		printf("%u.%u don't exist\n", tv.tv_sec, tv.tv_usec);
	

	// plus grand
//	n = ev_timeout_get_max)(&node);
//	ev_timeout_get_tv)(n, &tv);
//	printf("max=%u.%u\n", tv.tv_sec, tv.tv_usec);


/*
	tv.tv_sec=0;
	tv.tv_usec = 0;
	v = ev_timeout_add)(&node, &tv);

	tv.tv_usec = 1;
	ev_timeout_add)(&node, &tv);

	tv.tv_usec = 3;
	v = ev_timeout_add)(&node, &tv);

	tv.tv_usec = 4;
	ev_timeout_add)(&node, &tv);

	tv.tv_usec = 5;
	ev_timeout_add)(&node, &tv);

	ev_timeout_del)(v);
*/
}
