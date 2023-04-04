#ifndef __artd_SimpleSocketClient_h
#define __artd_SimpleSocketClient_h

// ARTD_HEADER_DOC_BEGIN
// ARTD_HEADER_DESCRIPTION: SimpleSocketClient for simple client server implementations
// ARTD_HEADER_DOC_END

#include <artd/jlib_net.h>
#include "artd/RcString.h"
#include "artd/InputStream.h"
#include "artd/OutputStream.h"
#include "artd/Thread.h"
#include "artd/Socket.h"

ARTD_BEGIN


class ARTD_API_JLIB_NET SimpleSocketClient
	: public ObjectBase
	, public Runnable
{
	ObjectPtr<Socket> socket_;
	ObjectPtr<OutputStream> ostr_;
	ObjectPtr<InputStream> istr_;
	RcString address_;
	int port_;

public:
	SimpleSocketClient(const char *address, int port);
	~SimpleSocketClient();

	int openConnection() {
		return(openConnection(address_.c_str(), port_, 5000));
	}

	int openConnection(const char *address, int port, int connectTimeout);
	// just a dummy real code goes in here.
	void run() {
		try {
			Thread::sleep(5000);
		}
		catch(...) {
		}
		closeConnection();
	}

	ObjectPtr<InputStream> getInputStream() {
		return(istr_);
	}
	ObjectPtr<OutputStream> getOutputStream() {
		return(ostr_);
	}

	void closeConnection()
	{
		if (ostr_ != nullptr) {
			ostr_->close();
			ostr_ = nullptr;
		}
		if (istr_ != nullptr) {
			istr_->close();
			istr_ = nullptr;
		}
		if (socket_ != nullptr) {
			socket_->close();
			socket_ = nullptr;
		}
	}
};

ARTD_END

#endif // __artd_SimpleSocketClient_h
