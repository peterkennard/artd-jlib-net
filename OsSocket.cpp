#include <artd/OsSocket.h>
#include "OsSocketImpl.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "artd/os_string.h"
#include "artd/Logger.h"
#include "artd/Thread.h"

#ifdef ARTD_WINDOWS
#else
	#include <strings.h>
    #include <sys/socket.h>
#endif


ARTD_BEGIN

#ifdef ARTD_WINDOWS
inline static void bzero(void *toClear, size_t bytes) {
	::memset( toClear, 0, bytes );
}
#endif



class OsSocketImplUDP :
	public OsSocketImpl 
{
public:
	OsSocketImplUDP();
public:
	bool init();
	int sendto(const char* buf, int len, const sockaddr* to, int tolen);
	int recv(char* buf, int len);
	int recvfrom(char* buf, int len, sockaddr* from, int* fromlen);
};

void getFirstLocalInterfaceIpv4Address(unsigned char *addr)
{
	memset(addr, 0, 4);
	#ifdef ARTD_WINDOWS
		char hostName[256];
		hostName[0] = 0;
		gethostname(hostName, sizeof(hostName));
		struct hostent *h = gethostbyname(hostName);
		if (h && h->h_addrtype == AF_INET)
			memcpy(addr, h->h_addr_list[0], 4);
	#else // Linux
		struct ifaddrs *ifaddr = 0;
		getifaddrs(&ifaddr);
		while (ifaddr)
		{
			if (ifaddr->ifa_addr->sa_family == AF_INET)
			{
				unsigned char *a = (unsigned char *) &((struct sockaddr_in *)ifaddr->ifa_addr)->sin_addr;
				if (a[0] != 127 || a[1] != 0 || a[2] != 0 || a[3] != 1)
				{
					memcpy(addr, a, 4);
					break;
				}
			}
			ifaddr = ifaddr->ifa_next;
		}
	#endif
}

#ifdef ARTD_WINDOWS
	WSADATA OsSocketImpl::wsData;
	int OsSocketImpl::instanceCount = 0;
#endif

bool OsSocketImpl::addRef()
{
#ifdef ARTD_WINDOWS
	// winsock init
	if ( instanceCount == 0 ) {
		if (	WSAStartup( MAKEWORD(1,1), &(wsData) ) != 0 &&
				WSAStartup( MAKEWORD(1,0), &(wsData) ) != 0 ) {
			fprintf(stderr, "Error: cannot start Winsock to use the network.\n" );
			return false;
		}
	}
	++instanceCount;
#endif
	return true;
}

void OsSocketImpl::releaseRef()
{
#ifdef ARTD_WINDOWS
	--instanceCount;
	if (instanceCount == 0) {
		WSACleanup();
	}
#endif
}

OsSocketImpl::OsSocketImpl()
{
	bzero( &sock_in, sizeof(sockaddr_in) );
    inputShutdown_ = false;
	sock = ~((sock_t)0); // note on windows this is unsigned
	recvTimeout_.tv_sec = 20;
	recvTimeout_.tv_usec = 0;
	sendTimeout_.tv_sec = 20;
	sendTimeout_.tv_usec = 0;
}

OsSocketImpl::~OsSocketImpl() {
	close();
	releaseRef();
}

int OsSocketImpl::getError()
{
#ifdef ARTD_WINDOWS
	return ::WSAGetLastError();
#else
	return errno;
#endif
}

RcString
OsSocketImpl::errorString() {
    #ifdef ARTD_WINDOWS
	int code = getError();
        return(RcString::format("winsock error 0x%x %d", code, code ));
/*
LPVOID lpMsgBuf;
	DWORD ret = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		ecode,
		0, // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);

	wchar_t *msg = L"";
	if(ret != 0) {
		msg = (wchar_t *)lpMsgBuf;
		wchar_t *p = ::wcsrchr(msg,'\n');
		if(p) p[-1] = 0;
	}
*/
#else
    char buff[128];
    uint32_t len = strerror_r(getError(),buff,sizeof(buff));
    if(len > sizeof(buff)) {
        len = sizeof(buff)-1;
    }
    buff[len] = 0;
    return(RcString::format("%s", buff));

#endif
}


bool OsSocketImpl::connect( const char* hostname, int port )
{
	sock_in.sin_family = AF_INET;
	sock_in.sin_port = htons( (unsigned short)abs(port) );

	hostent* host = gethostbyname( hostname );
	if( !host ) {
		sock_in.sin_addr.s_addr = inet_addr( hostname );

	#ifdef ARTD_WINDOWS
		if (sock_in.sin_addr.s_addr == ~((ULONG)0)) {
	#else
		if (sock_in.sin_addr.s_addr == ~((in_addr_t)0)) {
    #endif
				return false;
        }
	}
	else {
	#ifdef ARTD_WINDOWS
		memcpy( (char*)&sock_in.sin_addr, host->h_addr, host->h_length );
	#else
		bcopy( host->h_addr, (char*)&sock_in.sin_addr, host->h_length );
	#endif
	}

	if( port < 0 )
		return false;

	int rc = ::connect( sock, &sock_addr, sizeof( sockaddr_in ) );
    if(rc < 0) {
        // AD_LOG(error) << RcString() + "error on connect: \"" + errorString() + "\"");
        return(false);
    }	
    return(true);
}

bool OsSocketImpl::close()
{
    // AD_LOG(debug) << "closing socket 0x%x, %p %d", Thread::currentThreadId(), this, sock );
#ifdef ARTD_WINDOWS
    if(validSocket(sock)) {
        inputShutdown_ = true;
        ::shutdown(sock, SD_BOTH);
	    ::closesocket( sock );
    }
#else
    if(validSocket(sock)) {
        inputShutdown_ = true;
        ::shutdown(sock, SHUT_RD); // no SHUT_RDRW
        ::close( sock );
    }

/*
int val;
socklen_t len = sizeof(val);
if (getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, &val, &len) == -1)
    printf("fd %d is not a socket\n", fd);
else if (val)
    printf("fd %d is a listening socket\n", fd);
else
    printf("fd %d is a non-listening socket\n", fd);
*/

#endif
	bzero( &sock_in, sizeof(sockaddr_in) );
	sock = ~((sock_t)0); // note on windows this is unsigned
	return true;
}

bool OsSocketImpl::bind( int port )
{
    bzero((char *) &sock_in, sizeof(sockaddr_in));
	sock_in.sin_family = AF_INET;
	sock_in.sin_port = htons((unsigned short)port); 
	sock_in.sin_addr.s_addr = htonl( INADDR_ANY );

    // AD_LOG(debug) << " bind to socket 0x%x, %p %d", Thread::currentThreadId(), this, sock ); 
    clearError();
    int rc = ::bind( sock, (sockaddr*)&sock_in, sizeof(sock_in));
    if(rc < 0) {
        // AD_LOG(error) << RcString() + "error binding \"" + errorString()  + "\"");
        return(false);
    }
	return (true);
}

int OsSocketImpl::send( const char* buf, int len ) {
	return ::send(sock, buf, len, 0);
}

int OsSocketImpl::select_read(int timeout_secs, int timeout_usecs) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	timeval timeout;
	timeout.tv_sec = timeout_secs;
	timeout.tv_usec = timeout_usecs;
	int rc = select(1, &fds, NULL, NULL, &timeout);
	if (rc > 0 && ! FD_ISSET(sock, &fds)) {
		rc = -1;
	}
	return rc;
}
int OsSocketImpl::select_write(int timeout_secs, int timeout_usecs) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	timeval timeout;
	timeout.tv_sec = timeout_secs;
	timeout.tv_usec = timeout_usecs;
	int rc = select(1, NULL, &fds, NULL, &timeout);
	if (rc > 0 && ! FD_ISSET(sock, &fds)) {
		rc = -1;
	}
	return rc;
}

bool OsSocketImpl::setSendTimeout( long sec, long usec )
{
#ifdef ARTD_WINDOWS
	int t = (int)(sec * 1000.0 + usec / 1000.0);
	sendTimeout_.tv_sec = sec;
	sendTimeout_.tv_usec = usec;
#else
	struct timeval t;
	t.tv_sec = sec;
	t.tv_usec = usec;
#endif
	return 0 == ::setsockopt( sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&t, sizeof(t) );
}

bool OsSocketImpl::setRecvTimeout( long sec, long usec )
{
#ifdef ARTD_WINDOWS
	int t = (int)(sec * 1000.0 + usec / 1000.0);
	recvTimeout_.tv_sec = sec;
	recvTimeout_.tv_usec = usec;
#else
	struct timeval t;
	t.tv_sec = sec;
	t.tv_usec = usec;
#endif
	return 0 == ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&t, sizeof(t));
}

bool OsSocketImpl::setKeepAlive(bool on)
{
#ifdef ARTD_WINDOWS
	BOOL bon = on;
	return 0 == ::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *)&bon, sizeof(bon));
#else
    int keepalive = on;
    return(::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive , sizeof(keepalive )) >= 0);
#endif

/*
int keepcnt = 5;
int keepidle = 30;
int keepintvl = 120;

setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));
*/
}

int OsSocketImpl::getLocalPort()
{
	socklen_t len = sizeof( sock_in );
	int rc = getsockname( sock, &sock_addr, &len  ) ;
	if (rc == 0) {
		return ntohs(sock_in.sin_port);
	}
	else {
		return 0;
	}
}

OsSocketImplTCP::OsSocketImplTCP()
	: OsSocketImpl() {
}

bool 
OsSocketImplTCP::init() {
	if (!addRef()) {
		fprintf(stderr, "Couldn't initialize Winsock.\n");
		return false;
	}

	sock = ::socket( AF_INET, SOCK_STREAM, 0 );
    if (!validSocket(sock)) {
		// AD_LOG(error) << RcString() + "Couldn't create socket: \"" + errorString() + "\"");
		return false;
	}

	int flag = 1;
	::setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag) );
    // AD_LOG(debug) << " init socket success 0x%x %p %d", Thread::currentThreadId(), this, sock ); 
	return true;
}

bool 
OsSocketImplTCP::init(sock_t ext_sock, const sockaddr_in *ext_sock_in) {
	if (!addRef()) {
		fprintf(stderr, "Couldn't initialize Winsock.\n");
		return false;
	}
    // AD_LOG(debug) << "init2 socket %d", ext_sock );
	
    sock = ext_sock;
#ifdef ARTD_WINDOWS
	memcpy( &sock_in, ext_sock_in, sizeof(sockaddr_in) );
#else
	bcopy( ext_sock_in, (char*)&sock_in, sizeof(sockaddr_in) );
#endif

	int flag = 1;
	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag) );
	return true;
}

bool 
OsSocketImplTCP::listen( int backlog )
{
	if(::listen(sock, backlog) < 0) {
       // AD_LOG(error) << RcString() + "Listen failure: \"" + errorString() + "\""); 
       return(false);
    }
    return(true);
}

OsSocketImplTCP* 
OsSocketImplTCP::accept()
{
	sockaddr_in other_in;
	bzero( &other_in, sizeof(other_in) );
	socklen_t len = sizeof(other_in);

    clearError();
	sock_t other_sock = ::accept( sock, (sockaddr*)&other_in, &len );
    if (!validSocket(other_sock)) {
		// AD_LOG(error) << RcString() + "Error on accept: \"" + errorString() + "\"");
        return NULL;
	}
	int flag = 1;
	setsockopt( other_sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag) );

	OsSocketImplTCP* ret = new OsSocketImplTCP;
	if (!ret->init(other_sock, &other_in)) {
		delete ret;
		return NULL;
	}

	return ret;
}

int OsSocketImplTCP::recv( char* buf, int len )
{
	int count;
	int togo = len;
	char* bufp = buf;
	while( togo > 0 ) {
		count = ::recv( sock, bufp, togo, 0 );
		if( count < 0 ) return count;
		if( count == 0 ) return len - togo;
		togo -= count;
		bufp += count;
	}
	return len;
}
bool 
OsSocketImplTCP::isInputShutdown() const {
    if(!validSocket(sock)) {
        return(true);    
    }
    return(super::inputShutdown_);
}

// ***************************** UDP **************************/

OsSocketImplUDP::OsSocketImplUDP()
	: OsSocketImpl()
{
}

bool OsSocketImplUDP::init() {
	if (!addRef()) {
		fprintf(stderr, "Couldn't initialize Winsock.\n");
		return false;
	}
	sock = socket( AF_INET, SOCK_DGRAM, 0 );
    if (!validSocket(sock)) {
		// AD_LOG(error) << RcString() + "Couldn't create socket: \"" + errorString() + "\"");
		return false;
	}
#ifdef ARTD_WINDOWS
	//Requires at least Windows 2000 + SP2: See KB263823 re WinSock recvfrom now returns WSAECONNERESET
	//disable new behavior using IOCTL: SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
	DWORD dwBytesReturned = 0;
	BOOL bNewBehavior = FALSE;
	WSAIoctl(sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior),
		NULL, 0, &dwBytesReturned, NULL, NULL);
#endif
	return true;
}

int OsSocketImplUDP::sendto( const char* buf, int len, const sockaddr* to, int tolen )
{
	return ::sendto(sock, buf, len, 0, to, tolen);
}

int OsSocketImplUDP::recv( char* buf, int len )
{
	return ::recv(sock, buf, len, 0);
}

int OsSocketImplUDP::recvfrom( char* buf, int len, sockaddr* from, int* fromlen )
{
	socklen_t slen = fromlen ? *fromlen : 0;
	int res = ::recvfrom(sock, buf, len, 0, from, fromlen ? &slen : 0);
	if (fromlen) *fromlen = (int) slen;
	return res;
}

// ****** TCP interface  *******

OsSocketTCP::OsSocketTCP()
	: _impl(NULL)
{
}

OsSocketTCP::~OsSocketTCP()
{
	deleteZ(_impl);
	sock_ = NULL;
}

bool 
OsSocketTCP::init() {
	deleteZ(_impl);
	_impl = new OsSocketImplTCP;
	if (_impl == NULL) {
		return false;
	}
	else if (!_impl->init()) {
		delete _impl;
		_impl = NULL;
		sock_ = NULL;
		return false;
	}
	else {
		*((sock_t *)&sock_) = _impl->sock;
		return true;
	}
}

bool 
OsSocketTCP::init(OsSocketImplTCP* impl) {
	if (impl == NULL) {
		return false;
	}
	else {
		_impl = impl;
		*((sock_t *)&sock_) = _impl->sock;
		return true;
	}
}

bool OsSocketTCP::connect( const char* hostname, int port )
{
	if (_impl == NULL) {
		return false;
	}
	else {
		return _impl->connect(hostname, port);
	}
}

bool OsSocketTCP::close()
{
	if (_impl == NULL) {
		return false;
	}
	else {
		return _impl->close();
	}
}

bool
OsSocketTCP::shutdownInput() {
    if(_impl == NULL) {
        return(true);    
    } 
    _impl->inputShutdown_ = true;
#ifdef ARTD_WINDOWS
    ::shutdown(_impl->sock,SD_RECEIVE);
    return(true);
#else 
    return(::shutdown(_impl->sock,SHUT_RD) >= 0);
#endif
}

bool OsSocketTCP::bind( int port )
{
	if (_impl == NULL) {
		return false;
	}
	else {
		return _impl->bind(port);
	}
}

bool OsSocketTCP::listen( int backlog )
{
	if (_impl == NULL) {
		return false;
	}
	else {
		return(_impl->listen(backlog));
	}
}

int OsSocketTCP::send( const char* buf, int len )
{
	if (_impl == NULL) {
		return -1;
	}
	return _impl->send(buf, len);
}

int OsSocketTCP::recv( char* buf, int len )
{
	if (_impl == NULL) {
		return -1;
	}
	return _impl->recv(buf, len);
}

int OsSocketTCP::select_read(int timeout_secs, int timeout_usecs) {
	if (_impl == NULL) {
		return -1;
	}
	return _impl->select_read(timeout_secs, timeout_usecs);
}

int OsSocketTCP::select_write(int timeout_secs, int timeout_usecs) {
	if (_impl == NULL) {
		return -1;
	}
	return _impl->select_read(timeout_secs, timeout_usecs);
}

bool OsSocketTCP::setSendTimeout( long sec, long usec )
{
	if (_impl == NULL) {
		return false;
	}
	return _impl->setSendTimeout(sec, usec);
}

bool OsSocketTCP::setRecvTimeout( long sec, long usec )
{
	if (_impl == NULL) {
		return false;
	}
	return _impl->setRecvTimeout(sec, usec);
}
bool OsSocketTCP::setKeepAlive(bool on)
{
	if (_impl == NULL) {
		return(false);
	}
	return _impl->setKeepAlive(on);
}
int OsSocketTCP::getLocalPort()
{
	if (_impl == NULL) {
		return 0;
	}
	return _impl->getLocalPort();
}

int OsSocketTCP::getError()
{
	if (_impl == NULL) {
		return -1;
	}
	return _impl->getError();
}
RcString
OsSocketTCP::errorString() {
    return(_impl->errorString());
}


bool 
OsSocketTCP::isClosed() const
{
	return(_impl == NULL || !OsSocketImpl::validSocket(_impl->sock));
}
bool 
OsSocketTCP::isInputShutdown() const
{
	return(_impl == NULL || _impl->isInputShutdown());
}

// ****** UDP interface  *******

OsSocketUDP::OsSocketUDP()
	: _impl(NULL)
{
}

OsSocketUDP::~OsSocketUDP()
{
	deleteZ(_impl);
}

bool OsSocketUDP::init() {
	deleteZ(_impl);
	_impl = new OsSocketImplUDP();
	if (_impl == NULL) {
		return false;
	}
	else if (!_impl->init()) {
		delete _impl;
		_impl = NULL;
		return false;
	}
	else {
		return true;
	}
}

bool OsSocketUDP::connect( const char* hostname, int port )
{
	if (_impl == NULL) {
		return false;
	}
	return _impl->connect(hostname, port);
}

bool OsSocketUDP::close()
{
	if (_impl == NULL) {
		return false;
	}
	return _impl->close();
}

bool OsSocketUDP::bind( int port )
{
	if (_impl == NULL) {
		return false;
	}
	return _impl->bind(port);
}

int OsSocketUDP::send( const char* buf, int len )
{
	if (_impl == NULL) {
		return -1;
	}
	return _impl->send(buf, len);
}

int OsSocketUDP::sendto( const char* buf, int len, const sockaddr* to, int tolen )
{
	if (_impl == NULL) {
		return -1;
	}
	return _impl->sendto(buf, len, to, tolen);
}

int OsSocketUDP::recv( char* buf, int len )
{
	if (_impl == NULL) {
		return -1;
	}
	return _impl->recv(buf, len);
}

int OsSocketUDP::recvfrom( char* buf, int len, sockaddr* from, int* fromlen )
{
	if (_impl == NULL) {
		return -1;
	}
	return _impl->recvfrom(buf, len, from, fromlen);
}

int OsSocketUDP::select_read(int timeout_secs, int timeout_usecs) {
	if (_impl == NULL) {
		return -1;
	}
	return _impl->select_read(timeout_secs, timeout_usecs);
}

int OsSocketUDP::select_write(int timeout_secs, int timeout_usecs) {
	if (_impl == NULL) {
		return -1;
	}
	return _impl->select_read(timeout_secs, timeout_usecs);
}

bool OsSocketUDP::setSendTimeout( long sec, long usec )
{
	if (_impl == NULL) {
		return false;
	}
	return _impl->setSendTimeout(sec, usec);
}

bool OsSocketUDP::setRecvTimeout( long sec, long usec )
{
	if (_impl == NULL) {
		return false;
	}
	return _impl->setRecvTimeout(sec, usec);
}

int OsSocketUDP::getLocalPort()
{
	if (_impl == NULL) {
		return 0;
	}
	return _impl->getLocalPort();
}

int OsSocketUDP::getError()
{
	if (_impl == NULL) {
		return -1;
	}
	return _impl->getError();
}

RcString
OsSocketUDP::errorString() {
    return(_impl->errorString());
}


ARTD_END
