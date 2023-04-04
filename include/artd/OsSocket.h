#ifndef __artd_OsSocket_h
#define __artd_OsSocket_h

#include "artd/jlib_net.h"
#include "artd/RcString.h"
#include <stdexcept>

struct sockaddr;

ARTD_BEGIN

class OsSocketImplTCP;
class OsSocketImplUDP;
class SocketInputStream;
class SocketOutputStream;

/**
OsSocketTCP represents a TCP Socket.
This object cannot be copied. OsSockets that need to be passed to other objects
should be heap-allocated and passed by pointer.
After an OsSocket is constructed, initialize it by calling init().
If you do not call init, everything will be fine, but your socket will not work.
*/
class ARTD_API_JLIB_NET OsSocketTCP {
public:

	friend class SocketInputStream;
	friend class SocketOutputStream;
	friend class ServerSocket;

	/** Default-construct an OsSocketTCP. You must call init() after construction.*/
	OsSocketTCP();
	/** OsSockets will close the underlying socket on destruction.
	The last OsSocket to be destroyed will turn out the lights and shut the door.
	That is, it will call WSACleanup().
	*/
	~OsSocketTCP();
public:
	/** This function must be called after construction. You can also call it again later on
	to reinitialize. Your existing socket will be closed and a new socket will be created.*/
	bool init();
	/** Wrapper for connect() */
	bool connect(const char* hostname, int port);
	/** Wrapper for the system shutdown(), closesocket() or just close() on POSIX */
	bool close();
	/** Wrapper for the system bind() */
	bool bind(int port);
	/** Wrapper for the system listen() */
	bool listen(int backlog);
	/** Wrapper for the system send() */
	int send(const char* buf, int len);
	/** Wrapper for the system recv(). Will return only when exactly len bytes have been read, the other side
	sends EOF, or there is an error. */
	int recv(char* buf, int len);
	/** Wrapper for the system select(), with this socket in the read fd_set.
	Returns -1 if select returns > 0 but this socket is not set in the fd_set.
	*/
	int select_read(int timeout_secs, int timeout_usecs);
	/** Wrapper for select(), with this socket in the write fd_set.
	Returns -1 if select returns > 0 but this socket is not set in the fd_set.
	*/
	int select_write(int timeout_secs, int timeout_usecs);

	/** Wrapper for the system setsockopt() SO_SNDTIMEO */
	bool setSendTimeout(long sec, long usec);
	/** Wrapper for the system setsockopt() SO_RCVTIMEO */
	bool setRecvTimeout(long sec, long usec);
	/** Enable or disable Keep Alive packets */
	bool setKeepAlive(bool on);
	/** Return the local port */
	int getLocalPort();
	/** Get the last winsock or system error code. */
	int getError();
    RcString errorString();
	/** true if socket isn closed */
	bool isClosed() const;
    /** disable reading for connection */
	bool shutdownInput();
    /** return true if reading is shut down */
    bool isInputShutdown() const;  
private:
	bool init(OsSocketImplTCP* impl);
	OsSocketImplTCP* _impl;
	void *sock_;
private:
	OsSocketTCP(const OsSocketTCP&); //disallow
	void operator=(const OsSocketTCP&); //disallow
};

/**
OsSocketTCP represents a UDP Socket.
This object cannot be copied. OsSockets that need to be passed to other objects
should be heap-allocated and passed by pointer.
After an OsSocket is constructed, initialize it by calling init().
If you do not call init, everything will be fine, but your socket will not work.
*/
class ARTD_API_JLIB_NET OsSocketUDP {
public:
	/** Default-construct an OsSocketUDP. You must call init() after construction.*/
	OsSocketUDP();
	/** OsSockets will close the underlying socket on destruction.
	The last OsSocket to be destroyed will turn out the lights and shut the door.
	That is, it will call WSACleanup().
	*/
	~OsSocketUDP();
public:
	/** This function must be called after construction. You can also call it again later on
	to reinitialize. Your existing socket will be closed and a new socket will be created.*/
	bool init();
	/** Wrapper for the system connect() */
	bool connect(const char* hostname, int port);
	/** Wrapper for the system shutdown(), closesocket() or just close() on POSIX */
	bool close();
	/** Wrapper for the system bind() */
	bool bind(int port);
	/** Wrapper for the system send() */
	int send(const char* buf, int len);
	/** Wrapper for the system sendto() */
	int sendto(const char* buf, int len, const sockaddr* to, int tolen);
	/** Wrapper for the system recv(). Reads one whole datagram, up to size len */
	int recv(char* buf, int len);
	/** Wrapper for the system recvfrom(). Reads one whole datagram, up to size len */
	int recvfrom(char* buf, int len, sockaddr* from, int* fromlen);
	/** Wrapper for select(), with this socket in the read fd_set.
	Returns -1 if select returns > 0 but this socket is not set in the fd_set.
	*/
	int select_read(int timeout_secs, int timeout_usecs);
	/** Wrapper for select(), with this socket in the write fd_set.
	Returns -1 if select returns > 0 but this socket is not set in the fd_set.
	*/
	int select_write(int timeout_secs, int timeout_usecs);

	/** Wrapper for the system setsockopt() SO_SNDTIMEO */
	bool setSendTimeout(long sec, long usec);
	/** Wrapper for the system setsockopt() SO_RCVTIMEO */
	bool setRecvTimeout(long sec, long usec);

	/** Return the local port */
	int getLocalPort();
	/** Get the last winsock or system error code. */
	int getError();
    RcString errorString();

private:
	OsSocketImplUDP* _impl;

private:
	OsSocketUDP(const OsSocketUDP&); //disallow
	void operator=(const OsSocketUDP&); //disallow
};

ARTD_END

#endif //__artd_OsSocket_h
