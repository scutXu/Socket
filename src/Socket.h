#pragma once

#include <string>
#include <queue>
#include <functional>
#include "BufferList.h"

using std::string;
using std::queue;
using std::function;

class Socket;

typedef function<void(Socket, int)> AcceptCallback;
typedef function<void(int)> ConnectCallback;
typedef function<void(void *, int, int)> ReadCallback;
typedef function<void(int)> WriteCallback;

class Socket
{
public:
	enum State
	{
		CLOSED,
		OPENED,
		LISTENING,
		CONNECTING,
		CONNECTED,
	};

	Socket();
	Socket(int domain, int type, int protocol);
	Socket(Socket &&);
	Socket(const Socket &) = delete;
	Socket & operator= (const Socket &) = delete;
	~Socket();

	int getFD();

	State getState();

	int open(int domain, int type, int protocol);

	int bind(const char * ipAddress, uint16_t port);
	int bind(uint32_t ipAddress, uint16_t port);

	int listen(int backlog);

	void connect(const char * ipAddress, uint16_t port, ConnectCallback cb);
	void connect(uint32_t ipAddress, uint16_t port, ConnectCallback cb);

	void accept(AcceptCallback);

	void read(int size, ReadCallback cb);
	void readUntil(char delim, ReadCallback cb);

	void write(void * data, int size, WriteCallback cb);

	int close();

	bool waitToRead();
	bool waitToWrite();

	void doRead();
	void doWrite();

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
	ConnectCallback m_connectRequest;
	queue<ReadRequest> m_readRequests;
	queue<WriteRequest> m_writeRequests;
	BufferList m_readBuffer;
	BufferList m_writeBuffer;
};


