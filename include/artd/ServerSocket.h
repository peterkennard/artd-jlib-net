#ifndef __artd_ServerSocket_h
#define __artd_ServerSocket_h

#include "artd/Socket.h"

ARTD_BEGIN


class ARTD_API_JLIB_NET ServerSocket
	: public Socket 
{
	int listenPort_;
    typedef  Socket super;
public:

	ServerSocket(int listenPort);
	~ServerSocket();
	void setReuseAddress(bool yes);

	/**
	* Enable/disable {@link SocketOptions#SO_TIMEOUT SO_TIMEOUT} with the
	* specified timeout, in milliseconds.  With this option set to a non-zero
	* timeout, a call to accept() for this ServerSocket
	* will block for only this amount of time.  The option <B>must</B> be enabled
	* prior to entering the blocking operation to have effect.  The
	* timeout must be {@code > 0}.
	* A timeout of zero is interpreted as an infinite timeout.
	* @param timeout the specified timeout, in milliseconds
	* @exception SocketException if there is an error in
	* the underlying protocol, such as a TCP error.
	*/
	void setSoTimeout(int millis);

	ObjectPtr<Socket> accept();

	bool shutdownInput();
};

ARTD_END

#endif // __artd_ServerSocket_h
