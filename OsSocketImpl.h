#ifndef __artd_OsSocketImpl_ih
#define __artd_OsSocketImpl_ih

#include "artd/PlatformSocket.h"
#include "artd/RcString.h"

ARTD_BEGIN

class OsSocketImpl
{
public:
	friend class OsSocketTCP;
	friend class SocketInputStream;
	friend class SocketOutputStream;
	friend class Socket;
	friend class ServerSocket;
	
	OsSocketImpl();
	ARTD_API_JLIB_NET ~OsSocketImpl();
public:
	bool connect(const char* hostname, int port);
	bool close();
	bool bind(int port);
	int send(const char* buf, int len);
	int select_read(int timeout_secs, int timeout_usecs);
	int select_write(int timeout_secs, int timeout_usecs);

	bool setSendTimeout(long sec, long usec);
	bool setRecvTimeout(long sec, long usec);
	bool setKeepAlive(bool on);

	int getLocalPort();
	int getError();
	RcString errorString();

protected:

	sock_t sock;
	union
	{
		sockaddr_in sock_in;
		sockaddr sock_addr;
	};
	timeval recvTimeout_;
	timeval sendTimeout_;

    bool inputShutdown_;

#ifdef ARTD_WINDOWS

	static WSADATA wsData;
	static int instanceCount;

    inline void clearError() {
    }
    static inline bool validSocket(sock_t s) {
        return(s > 0 && s != ~((sock_t)0));  // windows this is unsigned
    }
#else 
    static inline bool validSocket(sock_t s) {
        return(s >= 0);
    }
    inline void clearError() {
        errno = 0;
    }
#endif

protected:
	static bool addRef();
	static void releaseRef();
};


class OsSocketImplTCP :
	public OsSocketImpl 
{
    typedef OsSocketImpl  super;
public:

	friend class Socket;
	friend class ServerSocket;

	OsSocketImplTCP();
public:
	bool init();
	bool listen(int backlog);
	ARTD_API_JLIB_NET OsSocketImplTCP* accept();
	int recv(char* buf, int len);
    bool isInputShutdown() const;
private:
	bool init(sock_t ext_sock, const sockaddr_in* ext_sock_in);

};


ARTD_END

#endif //__artd_OsSocketImpl_ih
