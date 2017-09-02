#include <assert.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utility>
#include <string.h>
#include "Socket.h"
#include "misc_error.h"

#define BLOCK_SIZE 1024

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

error_code Socket::open(int domain, int type, int protocol)
{
	assert(m_state == CLOSED);
	m_fd = socket(domain, type, protocol);
	if (m_fd >= 0) {
		m_state = OPENED;
        return error_code(0, std::generic_category());
	}
	assert(errno != 0);
    return error_code(errno, std::generic_category());
}

error_code Socket::bind(const char * ipAddress, uint16_t port)
{
	EndPoint ep(ipAddress, port);
	return bind(ep);
}


error_code Socket::bind(EndPoint & ep)
{
	assert(m_state == OPENED);
	int status = ::bind(m_fd, &ep.addr, sizeof(ep));
	if (status == 0) {
		m_state = BOUND;
		return error_code(0, std::generic_category());
	}
	assert(errno != 0);
	return error_code(errno, std::generic_category());
}

error_code Socket::listen(int backlog)
{
	assert(m_state == BOUND);
	int status = ::listen(m_fd, backlog);
	if (status == 0) {
		m_state = LISTENING;
		return error_code(0, std::generic_category());
	}
	assert(errno != 0);
	return error_code(errno, std::generic_category());
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
		cb(error_code(0, std::generic_category()));
	}
	else {
		if (errno == EINPROGRESS || errno == EINTR) {
			m_state = CONNECTING;
			m_connectRequest = cb;
		}
		else {
			assert(errno != 0);
			cb(error_code(errno, std::generic_category()));
			close();			
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
	rq.size = 0;
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

error_code Socket::close()
{
	if (m_fd >= 0) {
		int status = ::close(m_fd);
		if (status == 0) {
			m_state = CLOSED;
			m_fd = -1;
			return error_code(0, std::generic_category());
		}
		assert(errno != 0);
		return error_code(errno, std::generic_category());
	}
	//todo: already closed
}

bool Socket::waitToRead()
{
	return m_state == CONNECTING
		|| (m_state == CONNECTED && !m_readRequests.empty())
		|| (m_state == LISTENING && !m_acceptRequests.empty());
}

bool Socket::waitToWrite()
{
	return m_state == CONNECTED && !m_writeRequests.empty();
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
				cb(std::move(s), error_code(0, std::generic_category()));
			}
			else {
				if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ECONNABORTED) {
					//just wait for next connection arrived
				}
				else {
					AcceptCallback cb = m_acceptRequests.front();
					m_acceptRequests.pop();
					cb(Socket(), error_code(errno, std::generic_category()));
				}
			}
			
		}
	}
	else if (m_state == CONNECTING) {
		//check if connection successed
		int status = ::read(m_fd, nullptr, 0);
		if (status == 0) {
			m_state = CONNECTED;
			m_connectRequest(error_code(0, std::generic_category()));
		}
		else {
			assert(errno != 0);
			m_connectRequest(error_code(errno, std::generic_category()));
            close();
		}
	}
	else /*if (m_state == CONNECTED)*/ {
		while (!m_readRequests.empty()) {
			ReadRequest rq = m_readRequests.front();
			ssize_t bytesRead = 0;
			size_t originalSize = m_readBuffer.size();
			if (rq.size > 0) {
				assert(originalSize < rq.size);
				m_readBuffer.resize(rq.size);
				bytesRead = ::read(m_fd, &m_readBuffer[0], rq.size);

				if (bytesRead == 0) {
					void * data = originalSize == 0 ? NULL : (&m_readBuffer[0]);
					rq.cb(data, originalSize, misc_errc::eof);
					m_readRequests.pop();
					m_readBuffer.clear();
					while (!m_readRequests.empty()) {
						m_readRequests.front().cb(NULL, 0, misc_errc::eof);
						m_readRequests.pop();
					}
				}
				else if (bytesRead < 0) {
					if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
						m_readBuffer.resize(originalSize);
						break;
					}
					else {
						assert(errno != 0);
						void * data = originalSize == 0 ? NULL : (&m_readBuffer[0]);
						rq.cb(data, originalSize, error_code(errno, std::generic_category()));
						m_readRequests.pop();
						m_readBuffer.clear();
						close();
					}
				}
				else {
					if (bytesRead == rq.size) {
						rq.cb(&m_readBuffer[0], rq.size, error_code(0, std::generic_category()));
						m_readRequests.pop();
						m_readBuffer.clear();
					}
					else {
						m_readBuffer.resize(originalSize + bytesRead);
						break;
					}
				}
			}
			else {
				//todo
				assert(false);
			}
		}
	}
}

void Socket::doWrite()
{
    ssize_t bytesWritten = ::write(m_fd, &m_writeBuffer[0], m_writeBuffer.size());
    if (bytesWritten > 0) {
        WriteRequest wq;
        while (bytesWritten > 0 && bytesWritten > (wq = m_writeRequests.front()).size) {
            wq.cb(error_code(0, std::generic_category()));
            m_writeRequests.pop();
            bytesWritten -= wq.size;
        }
        if (bytesWritten > 0) {
            wq.size -= bytesWritten;
        }
    }
    else {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            assert(errno != 0);
            error_code ec(errno, std::generic_category());
            
            close();
        }
    }
}
