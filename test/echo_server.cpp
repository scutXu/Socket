#include <system_error>
#include "Log.h"
#include "Socket.h"

#define CHECK_ERROR(ec) \
	if(ec) { \
		std::string msg = ec.message(); \
		Log(msg.c_str()); \
		exit(0); \
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
	: m_socket(sock)
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
		Log(msg.c_str());
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
		Log(msg.c_str());
	}
}

vector<Session *> clients;
vector<Socket *> sockets;

int main()
{
	Socket server;
	sockets.push_back(&server);

	CHECK_ERROR(server.open(AF_INET, SOCK_STREAM, 0));
	CHECK_ERROR(server.bind("127.0.0.1", 3000));
	server.listen(100);

	AcceptCallback onAccept = [&](Socket && newSocket, const error_code & ec) {
		CHECK_ERROR(ec);
		Session * client = new Session(server.accept(onAccept));
		clients.push_back(client);
		sockets.push_back(&client->m_socket);
	};
	server.accept(onAccept);

	while (!sockets.empty()) {
		for (auto iter = sockets.begin(); iter != sockets.end(); ++iter) {
			if ((*iter)->waitToRead()) {
				(*iter)->doRead();
			}
			if ((*iter)->waitToWrite()) {
				(*iter)->doWrite();
			}
		}
	}
}
