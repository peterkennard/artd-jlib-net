#ifndef __artd_SocketOutputStream_cpp
#define __artd_SocketOutputStream_cpp

#include <errno.h>
#include "artd/jlib_net.h"
#include "artd/OsSocket.h"
#include "artd/OutputStream.h"
#include "artd/pointer_math.h"
#include "artd/Logger.h"
#include "artd/Socket.h"
#include "./OsSocketImpl.h"

ARTD_BEGIN


class ARTD_API_JLIB_NET SocketOutputStream
	: public OutputStream
{
	inline Socket &parent() const {
		return(*(Socket*)socket_);
	}
	inline OsSocketImpl &impl() {
		return(*(OsSocketImpl*)(((OsSocketTCP*)socket_)->_impl));
	}
	inline sock_t fd() {
		return(*(sock_t*)&(socket_->sock_));
	}
	const OsSocketTCP *socket_;
public:

	SocketOutputStream(OsSocketTCP *parent) : socket_(parent) {
	}
	~SocketOutputStream() {
	}
	/** OutputStream compliamce */
	int close() {
		::shutdown(fd(), SD_SEND);
		return(0);
	}
	unsigned int getFlags(unsigned int toGet) {
		if (toGet & fIsOpen) {
			if (socket_ != NULL && !socket_->isClosed()) {
				return(fIsOpen);
			}
		}
		return(0);
	}
	int flush() {
		return(0);
	}
	inline size_t getBytePosition() {
		return(0);
	}
	int put(unsigned char b)
	{
		if (::send(fd(), (const char *)&b, 1, 0) == 1) {
			return(1);
		}
		/* we try above first since below is more expensive
		* but below has logic to wait if a buffer is not available
		*/
		if (write(&b, 1) != 1) {
			return(-1);
		}
		return(1);
	}
	ptrdiff_t write(const void *buf, ptrdiff_t len)
	{
		// TRACE("writing \"%.*s\"", len, buf );
		ptrdiff_t totalSent = 0;
		for (;;)
		{
			ptrdiff_t ret = ::send(fd(), (char *)buf, (int)len, 0);
			if (ret >= 0) {
				totalSent += ret;
				if ((len -= ret) <= 0) {
					break;
				}
				buf = ARTD_OPTR(buf, ret);
			}
		#ifdef ARTD_WINDOWS
			else if (::WSAGetLastError() != WSAEWOULDBLOCK)
			{
				//AD_LOG(error) << "socket write error %d\n", ::WSAGetLastError()); // %s\n", errorString());
				goto error;
			}
		#else
			else if (errno != EWOULDBLOCK)
			{
                AD_LOG(error) << "socket write error " << errno; // << ", " << ; // %s\n", errorString());
				goto error;
			}
		#endif

			fd_set fd;
			FD_ZERO(&fd);
			FD_SET(this->fd(), &fd);

			fd_set fd2;
			FD_ZERO(&fd2);
			FD_SET(this->fd(), &fd2);

			// wait for more space
			if ((ret = ::select(1, NULL, &fd, &fd2, &impl().sendTimeout_)) != 1)
			{
			#ifdef ARTD_WINDOWS
				//AD_LOG(error) << "select error %d\n", ::WSAGetLastError()); // %s\n", errorString());
			#endif
				if (ret == 0 && totalSent == 0) {
			#ifdef ARTD_WINDOWS
					::WSASetLastError(WAIT_TIMEOUT);
			#else
					errno = ETIMEDOUT;
			#endif
					return(-1);
				}
			error:
				if (totalSent == 0) {
					return(-1);
				}
			#ifdef ARTD_WINDOWS
				::WSASetLastError(0);
			#else
				errno = 0;
			#endif
				break;
			}
		}
		return(totalSent);
	}

};


ARTD_END

#endif // __artd_SocketOutputStream_cpp

