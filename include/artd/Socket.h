#ifndef __artd_Socket_h
#define __artd_Socket_h

#include "artd/jlib_net.h"
#include "artd/InputStream.h"
#include "artd/OutputStream.h"
#include "artd/OsSocket.h"

ARTD_BEGIN


class ARTD_API_JLIB_NET Socket
	: public ObjectBase
	, public OsSocketTCP
{
	InputStream *is_ = nullptr;
	OutputStream *os_ = nullptr;

public:

	Socket();
	~Socket();

	ObjectPtr<InputStream> getInputStream() {	
		return(makeReferencingHandle(is_));
	}
	ObjectPtr<OutputStream> getOutputStream() {
		return(makeReferencingHandle(os_));
	}
};

ARTD_END

#endif // __artd_Socket_h
