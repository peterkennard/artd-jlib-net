#ifndef __artd_SocketInputStream_cpp
#define __artd_SocketInputStream_cpp

#include "artd/PlatformSocket.h"
#include <errno.h>
#include "artd/jlib_net.h"
#include "artd/OsSocket.h"
#include "artd/Socket.h"
#include "artd/InputStream.h"
#include "artd/pointer_math.h"
#include "artd/Logger.h"
#include "./OsSocketImpl.h"
ARTD_BEGIN

class ARTD_API_JLIB_NET SocketInputStream
	: public InputStream
{
	inline Socket &parent() const {
		return(*(Socket*)socket_);
	}

	inline OsSocketImpl &impl() {
		return(*(OsSocketImpl*)(((OsSocketTCP*)socket_)->_impl)  );
	}
	inline sock_t fd() {
        return(*(sock_t*)&(socket_->sock_));
	}    
	OsSocketTCP *socket_;
public:

	SocketInputStream(OsSocketTCP *parent) : socket_(parent) {
	}
	~SocketInputStream() {
	}
	/** InputStream compliamce */
	int close()
	{
	    socket_->shutdownInput();
		return(0);
	}
	unsigned int getFlags(unsigned int toGet) {
		if (toGet & fIsOpen) {
			if (socket_ != nullptr && !socket_->isInputShutdown()) {
				return(fIsOpen);
			}
		}
		return(0);
	}
	ptrdiff_t skip(ptrdiff_t bytes) {
		return(_skip(bytes));
	}
	inline size_t getBytePosition() {
		return(0);
	}

	int get(void)
	{
		unsigned char buf;
		if ((::recv(fd(), (char *)&buf, 1, 0)) == 1)
			return(buf);
		// read(p,len) will wait but is more expensive so
		// we try the above diretly first
		if (read(&buf, 1) != 1)
			return(-1);
		return(buf);
	}
	ptrdiff_t read(void *buf, ptrdiff_t len)
	{
		if (len == 0) {
			//LOGTRACE("length %d", len);
			return(0);
		}
		ptrdiff_t totalRcvd = 0;
	#ifdef ARTD_WINDOWS
		::WSASetLastError(0);
	#else
		// errno = 0;
	#endif

		for (;;)
		{
			// ptrdiff_t ret = ::recvfrom(fd(), (char *)buf, len, 0, 0, 0 );                
			ptrdiff_t ret = ::recv(fd(), (char *)buf, (int)len, 0);

			if (ret > 0)
			{
				totalRcvd += ret;
				if ((len -= ret) <= 0) {
					break;
				}
				buf = ARTD_OPTR(buf, ret);
			}
			else if (ret == 0) {
				goto error; // eof
			}
		#ifdef ARTD_WINDOWS
			else if (::WSAGetLastError() != WSAEWOULDBLOCK) {				
                //AD_LOG(error) << "socket read error: %s\n", socket_->errorString());
				goto error;
			}
		#else
			else if (errno != EWOULDBLOCK) {
                AD_LOG(error) << "socket read error " << errno << ", " << socket_->errorString();
				goto error;
			}
		#endif

			fd_set fd;
			FD_ZERO(&fd);
			FD_SET(this->fd(), &fd);

			if ((ret = ::select(1, &fd, NULL, NULL, &impl().recvTimeout_)) != 1)
			{
				if (ret == 0 && totalRcvd == 0)
				{
					//LOGTRACE("Socket timeout: %d seconds", impl().recvTimeout_.tv_sec);
				#ifdef ARTD_WINDOWS
					::WSASetLastError(WAIT_TIMEOUT);
				#else 
					errno = ETIMEDOUT;
				#endif
					return(-1);
				}
				/* TODO: if ret is 2 then the socket is in error */

				//LOGTRACE("read select error %d: %s\n", ret, socket_->errorString());

			#ifdef ARTD_WINDOWS
				switch (::WSAGetLastError())
				{
				case WSAEINVAL:  // seems select will return this when socket is shutdown :|
					::WSASetLastError(WSAESHUTDOWN);
				}
			#else
				switch (errno)
				{
				case EINVAL:  // seems select will return this when socket is shutdown :|
                                        // maybe windows only issue ?
					errno = ESHUTDOWN;
				}
			#endif
			error:
				if (totalRcvd == 0) {
					return(-1);
				}
				break;
			}
		}
		return(totalRcvd);
	}

};

ARTD_END

#endif // __artd_SocketInputStream_cpp

