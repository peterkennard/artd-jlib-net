#include "artd/PlatformSocket.h"
#include "artd/SimpleSocketClient.h"


ARTD_BEGIN

SimpleSocketClient::SimpleSocketClient(const char *address, int port) {
	address_ = address;


	port_ = port;
}
SimpleSocketClient::~SimpleSocketClient() {
}

int 
SimpleSocketClient::openConnection(const char *address, int port, int connectTimeout)
{
	socket_ = ObjectBase::make<Socket>();
    const char *msg = "";
	if (!socket_->init()) { msg = "init"; goto error; }
	if (!socket_->connect(address, port)) { msg="connect"; goto error; }
	if (!socket_->setRecvTimeout(0, 20000)) { msg="setRecvTimeout"; goto error; }
	if (!socket_->setSendTimeout(0, 20000)) { msg="setSendTimeout"; goto error; }
	if (!socket_->setKeepAlive(true)) { msg="setKeepAlive"; goto error; }

	// since we hold on to socket 
	istr_ = socket_->getInputStream();
	ostr_ = socket_->getOutputStream();

	AD_LOG(info) << "client connected";
	return(0);
error:
	RcString errmsg = socket_->errorString();
	AD_LOG(error) << "client connection " << msg << " failed: " << errmsg.c_str();
	return(-1);
}

ARTD_END

