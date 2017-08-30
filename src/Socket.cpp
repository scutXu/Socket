#include <assert.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utility>
#include <string.h>
#include "Socket.h"

Socket::Socket()
{
	m_fd = -1;
	m_state = CLOSED;
}

Socket::Socket(int domain, int type, int protocol)
{
	m_fd = -1;
	m_state = CLOSED;
	open(domain, type, protocol);
}

Socket::Socket(Socket && that)
{
	m_fd = that.m_fd;
	m_state = that.m_state;

	that.m_fd = -1;
	that.m_state = CLOSED;
}

Socket::~Socket()
{
	close();
}

int Socket::getFD()
{
	return m_fd;
}

Socket::State Socket::getState()
{
	return m_state;
}

void Socket::setNonBlocking()
{
	int flag = fcntl(m_fd, F_GETFL, 0);
	if (flag != -1) {
		fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
	}
}

int Socket::open(int domain, int type, int protocol)
{
	assert(m_state == CLOSED);
	m_fd = socket(domain, type, protocol);
	if (m_fd >= 0) {
		m_state = OPENED;
		return 0;
	}
	assert(errno != 0);
	return errno;
}

int Socket::bind(const char * ipAddress, uint16_t port)
{
	EndPoint ep(ipAddress, port);
	return bind(ep);
}


int Socket::bind(EndPoint & ep)
{
	assert(m_state == OPENED);
	int status = ::bind(m_fd, &ep.addr, sizeof(ep));
	if (status == 0) {
		m_state = BOUND;
		setNonBlocking();
		return 0;
	}
	assert(errno != 0);
	return errno;
}

int Socket::listen(int backlog)
{
	assert(m_state == BOUND);
	int status = ::listen(m_fd, backlog);
	if (status == 0) {
		m_state = LISTENING;
		return 0;
	}
	assert(errno != 0);
	return errno;
}

void Socket::connect(const char * ipAddress, uint16_t port, ConnectCallback cb)
{
	EndPoint ep(ipAddress, port);
	connect(ep, cb);
}

void Socket::connect(EndPoint & ep, ConnectCallback cb)
{
	assert(m_state == OPENED || m_state == BOUND);
	int status = ::connect(m_fd, &ep.addr, sizeof(ep));
	if (status == 0) {
		m_state = CONNECTED;
		cb(0);
	}
	else {
		if (errno == EINPROGRESS) {
			m_state = CONNECTING;
			m_connectRequest = cb;
		}
		else {
			int e = errno;
			close();
			assert(e != 0);
			cb(e);
		}
	}
}

void Socket::accept(AcceptCallback cb)
{
	assert(m_state == LISTENING);
	m_acceptRequests.push(cb);
}

void Socket::read(int size, ReadCallback cb)
{
	assert(m_state == CONNECTING || m_state == CONNECTED);
	ReadRequest rq;
	rq.cb = cb;
	rq.size = size;
	m_readRequests.push(rq);
}

void Socket::readUntil(char delim, ReadCallback cb)
{
	assert(m_state == CONNECTING || m_state == CONNECTED);
	ReadRequest rq;
	rq.cb = cb;
	rq.size = -1;
	rq.delim = delim;
	m_readRequests.push(rq);
}

void Socket::write(void * data, int size, WriteCallback cb)
{
	assert(m_state == CONNECTING || m_state == CONNECTED);

	size_t oldSize = m_writeBuffer.size();
	m_writeBuffer.resize(oldSize + size);
	memcpy(&m_writeBuffer[oldSize], data, size);

	WriteRequest wq;
	wq.cb = cb;
	wq.size = size;
	m_writeRequests.push(wq);
}

int Socket::close()
{
	if (m_fd >= 0) {
		int status = ::close(m_fd);
		m_fd = -1;
		m_state = CLOSED;
		if (status == 0) {
			return 0;
		}
		assert(errno != 0);
		return errno;
	}
	return 0;
}

bool Socket::waitToRead()
{
	return m_state == CONNECTING
		|| (m_state == CONNECTED && !m_readRequests.empty())
		|| (m_state == LISTENING && !m_acceptRequests.empty());
}

bool Socket::waitToWrite()
{
	return m_state == CONNECTED && !m_writeBuffer.empty();
}

void Socket::doRead()
{
	if (m_state == LISTENING) {
		if (!m_acceptRequests.empty()) {
			int fd = ::accept(m_fd, nullptr, nullptr);
			if (fd >= 0) {
				Socket s;
				s.m_fd = fd;
				s.m_state = CONNECTED;
				AcceptCallback cb = m_acceptRequests.front();
				m_acceptRequests.pop();
				cb(std::move(s), 0);
			}
			else {
				if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ECONNABORTED) {

				}
				else {
					AcceptCallback cb = m_acceptRequests.front();
					m_acceptRequests.pop();
					cb(Socket(), errno);
				}
			}
			
		}
	}
	else if (m_state == CONNECTING) {
		int status = ::read(m_fd, nullptr, 0);
		if (status == 0) {
			m_state = CONNECTED;
			m_connectRequest(0);
		}
		else {
			
            int e = errno;
            close();
            assert(e != 0);
            m_connectRequest(e);
		}
	}
	else if (m_state == CONNECTED) {

	}
	else {
		assert(false);
	}
}
