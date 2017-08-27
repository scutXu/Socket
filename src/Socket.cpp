#include <assert.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <utility>
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
		m_state == LISTENING;
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
		if (errno == EWOULDBLOCK || errno == EAGAIN) {
			m_state = CONNECTING;
			m_connectRequest = cb;
		}
		else {
			assert(errno != 0);
			cb(errno);
		}
	}
}

void Socket::accept(AcceptCallback cb)
{
	assert(m_state == LISTENING);
	int fd = ::accept(m_fd, nullptr, nullptr);
	if (fd >= 0) {
		Socket s;
		s.m_fd = fd;
		s.m_state = CONNECTED;
		cb(std::move(s), 0);
	}
	else {

	}
}