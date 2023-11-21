#include "artd/ServerSocket.h"
#include "./OsSocketImpl.h"

ARTD_BEGIN

ServerSocket::ServerSocket(int listenPort) 
	: listenPort_(listenPort)
{   
}
ServerSocket::~ServerSocket() {
}

void 
ServerSocket::setReuseAddress(bool yes) {

}
void 
ServerSocket::setSoTimeout(int millis) {

}
ObjectPtr<Socket> 
ServerSocket::accept() {
	if (_impl == nullptr) {
        init();
		if (_impl == nullptr) {
			return(nullptr);
		}
		if (!_impl->bind(listenPort_)) {
			_impl->close();
			_impl = nullptr;
			return(nullptr);
		}
		if (!_impl->listen(1)) {
			_impl->close();
			_impl = nullptr;
			return(nullptr);
		}
	}	
	OsSocketImplTCP* otherImpl = _impl->accept();	
	if (otherImpl == nullptr || isClosed() || (getError() != 0)) {
		deleteZ(otherImpl);
		return(nullptr);
	}
	ObjectPtr<Socket> ret = ObjectPtr<Socket>::make();
	if (!ret) {
		delete otherImpl;
		return nullptr;
	}
	if (!ret->init(otherImpl)) {
		ret = nullptr;
		return nullptr;
	}
	return(ret);
}

bool
ServerSocket::shutdownInput() {
	if (_impl == nullptr) {
		return(true);
	}
#ifdef ARTD_WINDOWS
	sock_t s = _impl->sock;
	_impl->sock = ~((sock_t)0);
	::shutdown(s, SD_RECEIVE); // on windows this does not abort a waiting accept call.
	::closesocket(s);          // this does.
	return(true);
#else 
	return(super::shutdownInput());
#endif
}


ARTD_END
