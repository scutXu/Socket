#include <system_error>
#include <algorithm>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include "Socket.h"
#include "Log.h"

#define CHECK_ERROR(ec) \
	if(ec) { \
		std::string msg = ec.message(); \
		log("%s", msg.c_str()); \
		exit(-1); \
	}

class SessionManager;

class Session
{
public:
	Session(Socket && sock, SessionManager & mgr);
	~Session();
	void onRead(void * data, int size, const error_code & ec);
	void onWrite(const error_code & ec);
	void onClose(const error_code & ec);

	Socket m_socket;
	SessionManager & m_mgr;
};

class SessionManager
{
public:
	void addSession(Session * s);
	void removeSession(Session * s);

	void clearClosedSessions();
private:
	vector<Session *> m_sessions;
	vector<Session *> m_sessionToRemove;
};

Session::Session(Socket && sock, SessionManager & mgr)
	: m_socket(std::move(sock))
	, m_mgr(mgr)
{
	m_mgr.addSession(this);
	m_socket.readUntil('\n', std::bind(&Session::onRead,
		this,
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3));
}

Session::~Session()
{
	m_socket.close();
}

void Session::onRead(void * data, int size, const error_code & ec)
{
	if (ec) {
		string msg = ec.message();
		log("onReadError:%s", msg.c_str());
	}
	else {
		m_socket.write(data, size, std::bind(&Session::onWrite, this, std::placeholders::_1));

		m_socket.readUntil('\n', std::bind(&Session::onRead,
			this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3));
	}
}

void Session::onWrite(const error_code & ec)
{
	if (ec) {
		string msg = ec.message();
		log("onWriteError:%s", msg.c_str());
	}
}

void Session::onClose(const error_code & ec)
{
	m_mgr.removeSession(this);
}

void SessionManager::addSession(Session * s)
{
	m_sessions.push_back(s);
}

void SessionManager::removeSession(Session * s)
{
	m_sessionToRemove.push_back(s);
}

void SessionManager::clearClosedSessions()
{
	while (!m_sessionToRemove.empty()) {
		log("remove session");
		Session * s = m_sessionToRemove[m_sessionToRemove.size() - 1];
		m_sessionToRemove.pop_back();
		auto iter = std::find(m_sessions.begin(), m_sessions.end(), s);
		assert(iter != m_sessions.end());
		m_sessions.erase(iter);
		delete s;
	}
}


int main()
{
    signal(SIGPIPE, SIG_IGN);
    
    Poller poller;
	SessionManager mgr;
	Socket server(poller);
    
    error_code ec = server.open(AF_INET, SOCK_STREAM, 0);
    CHECK_ERROR(ec);
    ec = server.bind("127.0.0.1", 3000);
	CHECK_ERROR(ec);
	server.listen(100);
    log("start listening...");

	AcceptCallback onAccept = [&](Socket && newSocket, const error_code & ec) {
		CHECK_ERROR(ec);
        log("new connection arrived:%d",newSocket.getFD());
		Session * client = new Session(std::move(newSocket), mgr);
		server.accept(onAccept);
	};
	server.accept(onAccept);

	poller.setLoopCallback(std::bind(&SessionManager::clearClosedSessions, &mgr));
    poller.run();
	
}
