#include "artd/Socket.h"
#include "./SocketInputStream.cpp"
#include "./SocketOutputStream.cpp"

ARTD_BEGIN

Socket::Socket() {   
	is_ = new SocketInputStream(this);
	os_ = new SocketOutputStream(this);
}
Socket::~Socket() {

	AD_LOG(info) << "deleting socket !!!";
	deleteZ(is_);
	deleteZ(os_);
}

ARTD_END

