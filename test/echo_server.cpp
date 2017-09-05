#include <system_error>
#include <stdlib.h>
#include "Log.h"
#include "Socket.h"

#define CHECK_ERROR(ec) \
	if(ec) { \
		std::string msg = ec.message(); \
		log("%s", msg.c_str()); \
		exit(-1); \
	}

class Session
{
public:
	Session(Socket && sock);
	~Session();
	void onRead(void * data, int size, const error_code & ec);
	void onWrite(const error_code & ec);

	Socket m_socket;
};

Session::Session(Socket && sock)
	: m_socket(std::move(sock))
{
	m_socket.readUntil('\n', std::bind(&Session::onRead,
		this,
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3));
}

Session::~Session()
{
	
}

void Session::onRead(void * data, int size, const error_code & ec)
{
	if (ec) {
		string msg = ec.message();
		log("onReadError:%s", msg.c_str());
	}
	else {
        log("read %d bytes::%s",size,(const char *)data);
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

vector<Session *> clients;

int main()
{
    signal(SIGPIPE, SIG_IGN);
    
    Poller poller;
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
		Session * client = new Session(std::move(newSocket));
		clients.push_back(client);
		server.accept(onAccept);
	};
	server.accept(onAccept);
    poller.run();
	
}
