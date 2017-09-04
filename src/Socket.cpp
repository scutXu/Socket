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

Socket::Socket(Poller & poller)
    :m_poller(poller)
{
	m_fd = -1;
	m_state = CLOSED;
}

Socket::Socket(int domain, int type, int protocol, Poller & poller)
    :m_poller(poller)
{
	m_fd = -1;
	m_state = CLOSED;
	open(domain, type, protocol);
}

Socket::Socket(Socket && that)
    :m_poller(that.m_poller)
{
	m_fd = that.m_fd;
	if (that.m_fd >= 0) {
		m_poller.replace(this, &that);
	}
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
		m_poller.add(this);
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
	int status = ::bind(m_fd, &(ep.addr), ep.getAddressLength());
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
			m_connectCallback = cb;
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

void Socket::write(const void * data, int size, WriteCallback cb)
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
		error_code ec(ECANCELED, std::generic_category());

		if (m_connectCallback != nullptr) {
			m_connectCallback(ec);
		}
		while (!m_acceptRequests.empty())
		{
			(m_acceptRequests.front())(Socket(m_poller), ec);
			m_acceptRequests.pop();
		}

		while (!m_readRequests.empty())
		{
			m_readRequests.front().cb(nullptr, 0, ec);
			m_readRequests.pop();
		}

		while (!m_writeRequests.empty())
		{
			m_writeRequests.front().cb(ec);
			m_writeRequests.pop();
		}

		m_writeBuffer.clear();
		m_readBuffer.clear();

		m_poller.remove(this);
		int status = ::close(m_fd);
		m_state = CLOSED;
		m_fd = -1;
		if (status == 0) {
			return error_code(0, std::generic_category());
		}
		assert(errno != 0);
		return error_code(errno, std::generic_category());
	}

	return error_code(EBADF, std::generic_category());
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
				Socket s(m_poller);
				s.m_fd = fd;
				s.m_state = CONNECTED;
				m_poller.add(&s);
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
					cb(Socket(m_poller), error_code(errno, std::generic_category()));
				}
			}
			
		}
	}
	else if (m_state == CONNECTING) {
		//check if connection successed
		int status = ::read(m_fd, nullptr, 0);
		if (status == 0) {
			m_state = CONNECTED;
			m_connectCallback(error_code(0, std::generic_category()));
			m_connectCallback = nullptr;
		}
		else {
			assert(errno != 0);
			m_connectCallback(error_code(errno, std::generic_category()));
			m_connectCallback = nullptr;
            close();
		}
	}
	else /*if (m_state == CONNECTED)*/ {
		while (!m_readRequests.empty()) {
			ReadRequest rq = m_readRequests.front();
			ssize_t bytesRead = 0;
			size_t originalSize = m_readBuffer.size();
			size_t expectedBytesRead = 0;
			if (rq.size > 0) {
				assert(originalSize < rq.size);
				expectedBytesRead = rq.size - originalSize;
			}
			else {
				expectedBytesRead = BLOCK_SIZE;
			}
			m_readBuffer.resize(originalSize + expectedBytesRead);
			bytesRead = ::read(m_fd, &m_readBuffer[originalSize], expectedBytesRead);
			if (bytesRead == 0) {
				//eof
				void * data = originalSize == 0 ? nullptr : (&m_readBuffer[0]);
				rq.cb(data, originalSize, std::make_error_code(misc_errc::eof));
				m_readRequests.pop();
				m_readBuffer.clear();
				while (!m_readRequests.empty()) {
					m_readRequests.front().cb(nullptr, 0, std::make_error_code(misc_errc::eof));
					m_readRequests.pop();
				}
			}
			else if (bytesRead < 0) {
				//error
				if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
					m_readBuffer.resize(originalSize);
					break;
				}
				else {
					assert(errno != 0);
					void * data = originalSize == 0 ? nullptr : (&m_readBuffer[0]);
					rq.cb(data, originalSize, error_code(errno, std::generic_category()));
					m_readRequests.pop();
					m_readBuffer.clear();
					close();
				}
			}
			else {
				m_readBuffer.resize(originalSize + bytesRead);
				if (rq.size > 0) {
					if (bytesRead == expectedBytesRead) {
						rq.cb(&m_readBuffer[0], rq.size, error_code(0, std::generic_category()));
						m_readRequests.pop();
						m_readBuffer.clear();
					}
					else {
						//too few data
						break;
					}
				}
				else {
					//check if delim read
					size_t i = originalSize;
					for (; i < originalSize + bytesRead; ++i) {
						if (m_readBuffer[i] == rq.delim) {
							break;
						}
					}

					if (i < originalSize + bytesRead) {
						//delim read
						rq.cb(&m_readBuffer[0], i + 1, error_code(0, std::generic_category()));
						m_readRequests.pop();
						m_readBuffer.erase(m_readBuffer.begin(), m_readBuffer.begin() + i + 1);
					}
					else {
						if (bytesRead == expectedBytesRead) {

						}
						else {
							//too few data
							break;
						}
					}
				}
			}
		}
	}
}

void Socket::doWrite()
{
    ssize_t bytesWritten = ::write(m_fd, &m_writeBuffer[0], m_writeBuffer.size());
    if (bytesWritten > 0) {
		m_writeBuffer.erase(m_writeBuffer.begin(), m_writeBuffer.begin() + bytesWritten);
        WriteRequest wq;
        while (bytesWritten > 0 && bytesWritten > (wq = m_writeRequests.front()).size) {
            wq.cb(error_code(0, std::generic_category()));
            m_writeRequests.pop();
            bytesWritten -= wq.size;
        }
		wq.size -= bytesWritten;
    }
    else {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            assert(errno != 0);
			m_writeRequests.front().cb(error_code(errno, std::generic_category()));
			m_writeRequests.pop();
            close();
        }
    }
}
