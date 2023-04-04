#ifndef ARTD_PLATFORM_SOCKET_H
#define ARTD_PLATFORM_SOCKET_H

#include "artd/jlib_net.h"

#ifdef ARTD_WINDOWS
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include "artd/platform_specific.h"
	#include <winsock2.h>
	#pragma comment( lib, "Ws2_32.lib")   
	#include <io.h>
	typedef SOCKET sock_t;
	typedef int socklen_t;
#else
	#include "artd/jlib_base.h"
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <ifaddrs.h>
	#include <errno.h>
	typedef int sock_t;

ARTD_BEGIN
	static const int SD_RECEIVE  = SHUT_RD;
	static const int SD_SEND  = SHUT_WR;
ARTD_END

#endif

#endif // ARTD_PLATFORM_SOCKET_H
