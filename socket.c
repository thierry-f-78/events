#define __BIND_C__

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

// #include "conf.h"
#include "log.h"

static char *protos[]= {
	[AF_UNIX]  = "AF_UNIX",
	[AF_INET]  = "AF_INET",
	[AF_INET6] = "AF_INET6"
};

int bind_init(char *socket_name, int maxconn) {
	int ret_code;
	int listen_socket;
	int conf_socket_type = -1;
	struct sockaddr_storage conf_adress;
	int one = 1;
	char *error;
	struct stat statbuf;
	int already_binded = 0;
	int i;
	char *conf_addr;
	int conf_port;
	char *path;
	struct fcgi *fcgi;
	
	memset(&conf_adress, 0, sizeof(struct sockaddr_storage));

	// -----------------------------------
	//  detect address type 
	// -----------------------------------

	// default socket is 0
	if (socket_name == NULL) {
		conf_socket_type = AF_UNIX;
		listen_socket = 0;
		already_binded = 1;
		goto start_non_bloc;
	}

	// copy data
	path = strdup(socket_name);

	// on cherche le separateur ':'
	for(i=strlen(path)-1; i>0; i--)
		if (path[i]==':')
			break;

	// si on l'à trouvé, on sépare l'indicateur de reseau du port
	if (path[i]==':') {
		path[i] = '\0';
		conf_addr = path;
		conf_port = strtol(&path[i+1], &error, 10);
		if (*error != '\0') {
			logmsg(LOG_ERR, "\"%s\" is not a port", path, &path[i+1]);
			exit(1);
		}
	}

	// si on ne trouve pas de separateur, ben c'est
	// que c'est une socket unxi
	else
		conf_socket_type = AF_UNIX;

	// AF_UNIX detected create pipe and buils structur
	if ( conf_socket_type == AF_UNIX ) {
		((struct sockaddr_un *)&conf_adress)->sun_family = AF_UNIX;
		strncpy(((struct sockaddr_un *)&conf_adress)->sun_path, path,
		        sizeof(((struct sockaddr_un *)&conf_adress)->sun_path) - 1);

		goto end_of_building_address;
	}

	// AF_INET detected, builds address structur
	ret_code = inet_pton(AF_INET, conf_addr,
	      (const void *)&((struct sockaddr_in *)&conf_adress)->sin_addr);
	if (ret_code > 0) {
		conf_socket_type = AF_INET;
		((struct sockaddr_in *)&conf_adress)->sin_port = htons(conf_port);
		goto end_of_building_address;
	}

	// AF_INET6 detected, builds address structur
	ret_code = inet_pton(AF_INET6, conf_addr,
	      (const void *)&((struct sockaddr_in6 *)&conf_adress)->sin6_addr);
	if (ret_code > 0) {
		conf_socket_type = AF_INET6;
		((struct sockaddr_in6 *)&conf_adress)->sin6_port = htons(conf_port);
		goto end_of_building_address;
	}

	// adress format error
	logmsg(LOG_ERR, "address \"%s\" unavalaible", socket_name);
	exit(1);

	end_of_building_address:

	// open socket for AF_UNIX
	if (conf_socket_type == AF_UNIX) {
		listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
		if (listen_socket < 0) {
			logmsg(LOG_CRIT, "socket(AF_UNIX, SOCK_STREAM, 0)[%d]: %s",
			       errno, strerror(errno));
			exit(1);
		}
	}

	// open socket for network protocol
	else {
		listen_socket = socket(conf_socket_type, SOCK_STREAM, IPPROTO_TCP);
		if (listen_socket < 0) {
			logmsg(LOG_CRIT, "socket(%s, SOCK_STREAM, IPPROTO_TCP)[%d]: %s",
			       protos[conf_socket_type], errno, strerror(errno));
			exit(1);
		}
	}

	start_non_bloc:

	// set non block opt
	ret_code = fcntl(listen_socket, F_SETFL, O_NONBLOCK);
	if (ret_code == -1) {
		logmsg(LOG_CRIT, "fcntl(F_SETFL, O_NONBLOCK)[%d]: %s",
		       errno, strerror(errno));
		exit(1);
	}

	// tcp no delay
	if (conf_socket_type == AF_INET6 || conf_socket_type == AF_INET ) {
		ret_code = setsockopt(listen_socket, IPPROTO_TCP, TCP_NODELAY,
		                      (char *)&one, sizeof(one));
		if (ret_code == -1) {
			logmsg(LOG_CRIT, "setsockopt(IPPROTO_TCP, TCP_NODELAY)[%d]: %s",
			       errno, strerror(errno));
			exit(1);
		}
	}

	// reuse addr
	if (conf_socket_type == AF_INET6 || conf_socket_type == AF_INET ) {
		ret_code = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR,
		                      (char *)&one, sizeof(one));
		if (ret_code == -1) {
			logmsg(LOG_CRIT, "setsockopt(SOL_SOCKET, SO_REUSEADDR)[%d]: %s",
			       errno, strerror(errno));
			exit(1);
		}
	}

	// delete socket if already exist
	if ( conf_socket_type == AF_UNIX && already_binded == 0) {
		unlink(path);
	}

	// bind address
	if (already_binded == 0) {
		switch (conf_socket_type) {
			case AF_INET:
				ret_code = bind(listen_socket,
				                (struct sockaddr *)&conf_adress,
				                sizeof(struct sockaddr_in));
				break;

			case AF_INET6:
				ret_code = bind(listen_socket,
				                (struct sockaddr *)&conf_adress,
				                sizeof(struct sockaddr_in6));
				break;
		
			case AF_UNIX:
				ret_code = bind(listen_socket,
				                (struct sockaddr *)&conf_adress,
				                sizeof(struct sockaddr_un));
				break;
		}
		if (ret_code == -1) {
			logmsg(LOG_CRIT, "bind(\"%s\")[%d]: %s",
			       socket_name, errno, strerror(errno));
			exit(1);
		}
	}

	// listen
	ret_code = listen(listen_socket, maxconn);
	if (ret_code == -1) {
		logmsg(LOG_CRIT, "listen(%d)[%d]: %s",
		       maxconn, errno, strerror(errno));
		exit(1);
	}

	// return filedescriptor
	return listen_socket;
}

int bind_accept(int listen_socket, struct sockaddr_storage *addr) {
	int fd;
	int len;
	int ret_code;
	int one = 1;

	len = sizeof(addr);
	fd = accept(listen_socket, (struct sockaddr *)addr, &len);
	if (fd == -1) {
		switch (errno) {
			case EAGAIN:
			case EINTR:
			case ECONNABORTED:
				return -1;       /* nothing more to accept */
			case ENFILE:
				logmsg(LOG_ERR, "system FD limit reached at %%d. "
				                "Please check system tunables.");
				return -1;
			case EMFILE:
				logmsg(LOG_ERR, "process FD limit reached at %%d. "
				                "Please check 'ulimit' and restart.");
				return -1;
			case ENOBUFS:
			case ENOMEM:
				logmsg(LOG_ERR, "system memory limit reached at %%d "
				                "sockets. Please check system tunables.");
				return -1;
			default:
				logmsg(LOG_ERR, "accept[%d]: %s", errno, strerror(errno));
				return -1;
		}
	}

	ret_code = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (ret_code == -1) {
		logmsg(LOG_ERR, "fcntl(F_SETFL, O_NONBLOCK)[%d]: %s",
		       errno, strerror(errno));
		close(fd);
		return -1;
	}

	if (((struct sockaddr *)addr)->sa_family != AF_UNIX) {
		ret_code = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		                      (char *)&one, sizeof(one));
		if (ret_code == -1) {
			logmsg(LOG_ERR, "setsockopt(IPPROTO_TCP, TCP_NODELAY)[%d]: %s",
			       errno, strerror(errno));
			close(fd);
			return -1;
		}

		ret_code = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
		                      (char *) &one, sizeof(one));
		if (ret_code == -1) {
			logmsg(LOG_ERR, "setsockopt(SOL_SOCKET, SO_KEEPALIVE)[%d]: %s",
			       errno, strerror(errno));
			close(fd);
			return -1;
		}
	}

	// return file descripteur
	return fd;
}

