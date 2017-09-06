#pragma once

#include <string>
#include <vector>
#include <queue>
#include <functional>
#include <system_error>
#include "BufferList.h"
#include "EndPoint.h"
#include "Poller.h"

using std::string;
using std::vector;
using std::queue;
using std::function;
using std::error_code;

class Socket;

typedef function<void(Socket &&, const error_code &)> AcceptCallback;
typedef function<void(const error_code &)> ConnectCallback;
typedef function<void(void *, int, const error_code &)> ReadCallback;
typedef function<void(const error_code &)> WriteCallback;
typedef function<void(const error_code &)> CloseCallback;

class Socket
{
public:
	enum State
	{
		CLOSED,
		OPENED,
		BOUND,
		LISTENING,
		CONNECTING,
		CONNECTED,
	};

	Socket(Poller & poller);
	Socket(int domain, int type, int protocol, Poller & poller);
	Socket(Socket &&);
	Socket(const Socket &) = delete;
	Socket & operator= (const Socket &) = delete;
	~Socket();

	int getFD();

	State getState();

	void setNonBlocking();

	error_code open(int domain, int type, int protocol);

	error_code bind(const char * ipAddress, uint16_t port);
	//int bind(uint32_t ipAddress, uint16_t port);
	//int bind(uint64_t ipAddress, uint16_t port);
	error_code bind(EndPoint & ep);


	error_code listen(int backlog);

	void connect(const char * ipAddress, uint16_t port, ConnectCallback cb);
	//void connect(uint32_t ipAddress, uint16_t port, ConnectCallback cb);
	void connect(EndPoint & ep, ConnectCallback cb);

	void accept(AcceptCallback);

	void read(int size, ReadCallback cb);
	void readUntil(char delim, ReadCallback cb);

	void write(const void * data, int size, WriteCallback cb);

	void close();

	bool waitToRead();
	bool waitToWrite();

	void doRead();
	void doWrite();

	void setCloseCallback(CloseCallback cb);

private:
	struct ReadRequest
	{
		int size;
		char delim;
		ReadCallback cb;
	};

	struct WriteRequest
	{
		int size;
		WriteCallback cb;
	};

private:
	int m_fd;
	State m_state;
	queue<AcceptCallback> m_acceptRequests;
	ConnectCallback m_connectCallback;
	queue<ReadRequest> m_readRequests;
	queue<WriteRequest> m_writeRequests;
	CloseCallback m_closeCallback;
	vector<uint8_t> m_readBuffer;
	vector<uint8_t> m_writeBuffer;
    
    Poller & m_poller;

	//BufferList m_readBuffer;
	//BufferList m_writeBuffer;
};


