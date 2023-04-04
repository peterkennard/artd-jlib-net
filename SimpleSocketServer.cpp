#include "artd/SimpleSocketServer.h"
#include "artd/ObjLinkedList.h"

ARTD_BEGIN

SimpleSocketServerBase::SimpleSocketServerBase(int listenPort)
	: listenPort_(listenPort)
{
	debug_ = false;
	sessions_ = ObjectBase::make<ObjLinkedList<ServerSession>>();
}
SimpleSocketServerBase::~SimpleSocketServerBase() {
}

void 
SimpleSocketServerBase::addSession(ObjectPtr<ServerSession> &c) {
	synchronized(clientListLock_)
		sessions_->add(c);
}


void 
SimpleSocketServerBase::removeSession(ServerSession *c) {
	synchronized(clientListLock_);
	if (sessions_->remove(c)) {
		AD_LOG(debug) << "session " << c->getName() << " removed";
	}
}

void 
SimpleSocketServerBase::stopAllSessions() {
	for (;;) {
		ServerSession *c;
		{
			synchronized(clientListLock_);
			if (sessions_->isEmpty()) {
				break;
			}
			c = sessions_->peekFirst().get();
		}
		c->shutdown();
	}
}

void 
SimpleSocketServerBase::runInCurrentThread() {
	running_ = true;
	run();
}

void 
SimpleSocketServerBase::run() {

	// int count = -1;
	while (running_) {

		//try {

		listenSocket_ = ObjectBase::make<ServerSocket>(listenPort_);
		listenSocket_->setReuseAddress(true);
		listenSocket_->setSoTimeout(20000);

		//	if (debug_) 
		AD_LOG(info) << "listening on port " << listenPort_;
		startSignal_.signal();

		while (running_) {

			ObjectPtr<Socket> connectionSocket = listenSocket_->accept();
			ObjectPtr<ServerSession> conn;
			if (!running_) {
				break;
			}
			if (!connectionSocket) {
				goto acceptError;
			}
			conn = getNewSession();
			conn->setServer(this);

			conn->setSocket(connectionSocket);
			addSession(conn);
			if (!conn->start()) {
				goto acceptError;
			}
			AD_LOG(info) << "connection accepted " << (void *)(connectionSocket.get());
			continue;
		acceptError:
			AD_LOG(debug) << "failure accepting connection";
		}
	}

	if (listenSocket_) {
		stopAllSessions();
		listenSocket_->close();
        listenSocket_ = nullptr;
	}
	// AD_LOG(debug) << "listen thread exiting");
	running_ = false;
}


bool 
SimpleSocketServerBase::start() {
	
	ObjectPtr<Thread> th = ObjectBase::make<Thread>(sharedFromThis(this));

	listenThread_ = th;
	running_ = true;
	listenThread_->start();

    int ret = startSignal_.waitOnSignal(5000);
	if (ret  != 0) {
		shutdown();
	}
	return(ret == 0);
}

void 
SimpleSocketServerBase::shutdown() {

 	if(listenThread_) AD_LOG(debug) << listenThread_->getName() << " calling server->shutdown()";

	if (listenThread_ && running_) {
		running_ = false;

		if (listenSocket_) {
       	    listenSocket_->shutdownInput();
		}
		stopAllSessions();
		if (listenSocket_) {
			listenSocket_->close();
		}

		AD_LOG(debug) << listenThread_->getName() << " before join";

		listenThread_->join(20000);

		listenSocket_ = nullptr;
		listenThread_ = nullptr;
	}
	else {
		stopAllSessions();
	}
}


bool 
SimpleSocketServerBase::ServerSession::start() {
	running_ = true;
	myThread_->start();
	return(true);
}


void 
SimpleSocketServerBase::ServerSession::shutdown() {
	if (running_) {
		running_ = false;
		if (myThread_.ptr() != Thread::currentThread()) {
			myThread_->join(5000);
			myThread_->interrupt();
		}
	}
	if (socket_) {
		socket_->close();
		socket_ = nullptr;
	}
	server_->removeSession(this);
}


ARTD_END


