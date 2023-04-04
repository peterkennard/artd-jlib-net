#ifndef __artd_SimpleSocketServer_h
#define __artd_SimpleSocketServer_h

// ARTD_HEADER_DOC_BEGIN
// ARTD_HEADER_DESCRIPTION: SimpleSocketServer for simple client server implementations
// ARTD_HEADER_DOC_END

#include "artd/jlib_core.h"
#include "artd/RcString.h"
#include "artd/InputStream.h"
#include "artd/OutputStream.h"
#include "artd/Thread.h"
#include "artd/ServerSocket.h"
#include "artd/ObjLinkedList.h"
#include "artd/synchronized.h"
#include "artd/CriticalSection.h"
#include "artd/WaitableSignal.h"

ARTD_BEGIN


class ARTD_API_JLIB_NET SimpleSocketServerBase
	: public ObjectBase
	, public Runnable
{
	RcString address_;
	int listenPort_;
	ObjectPtr<Thread> listenThread_;
	ObjectPtr<ServerSocket> listenSocket_;
	WaitableSignal startSignal_;
	bool running_;
	bool debug_;

protected:
	SimpleSocketServerBase(int listenPort);
public:
	~SimpleSocketServerBase();

	class ARTD_API_JLIB_NET ServerSession
		: public ObjectBase
		, public Runnable
	{
		friend class SimpleSocketServerBase;

		SimpleSocketServerBase *server_;
		ObjectPtr<Socket> socket_;
		ObjectPtr<Thread> myThread_;
	
		void setServer(SimpleSocketServerBase *s) {
			server_ = s;
		}

		void setSocket(ObjectPtr<Socket>  &s) {
			socket_ = s;
		}

	protected:
		bool running_;

	public:

		ServerSession() {
			ObjectPtr<Thread> temp = ObjectBase::make<Thread>(sharedFromThis(this));
			myThread_ = temp;  // TODO: release ref held by thread ??
		}
		SimpleSocketServerBase *getServer() {
			return(server_);
		}
		Socket *getSocket() {
			return(socket_.get());
		}

	protected:
		bool start();
		virtual void shutdown();
		inline InputStream *getInputStream() {
			return (socket_->getInputStream().get());
		}
		inline OutputStream *getOutputStream() {
			return (socket_->getOutputStream().get());
		}
		inline bool isRunning() {
			return (running_);
		}
		virtual void run() = 0;
	
	public:

		RcString getName() {
			return(RcString::format("client %p", this));
		}

	};

	CriticalSection clientListLock_;

	virtual  ObjectPtr<ServerSession> getNewSession() = 0;

	ObjectPtr<ObjLinkedList<ServerSession>> sessions_;

	void addSession(ObjectPtr<ServerSession> &c);
	void removeSession(ServerSession *c);
	void stopAllSessions();

public:

	inline RcString getName() {
		return("xx"); //  (getClass().getSimpleName());
	}

	void runInCurrentThread();
	void run();
	bool start();
	unsigned int release();
	void shutdown();
};

template<class SubclassT>
class SimpleSocketServer
	: public SimpleSocketServerBase 
{
public:
	SimpleSocketServer(int listenPort) 
		: SimpleSocketServerBase(listenPort) {
	}
	~SimpleSocketServer() {
	}
};


ARTD_END

#endif // __artd_SimpleSocketServer_h
