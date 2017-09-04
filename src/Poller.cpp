#include <algorithm>
#include <assert.h>
#include "Poller.h"
#include "Socket.h"

void Poller::add(Socket *socket)
{
	m_socksToAdd.push_back(socket);
}

void Poller::remove(Socket *socket)
{
	
    auto iter = std::find(m_sockRefs.begin(), m_sockRefs.end(), socket);
    if(iter != m_sockRefs.end()) {
		*iter = nullptr;
		m_removeHappened = true;
    }
	else {
		iter = std::find(m_socksToAdd.begin(), m_socksToAdd.end(), socket);
		assert(iter != m_socksToAdd.end());
		m_socksToAdd.erase(iter);
	}
}

void Poller::replace(Socket * newSocket, Socket * oldSocket)
{
	auto iter = std::find(m_sockRefs.begin(), m_sockRefs.end(), oldSocket);
	if (iter != m_sockRefs.end()) {
		*iter = newSocket;
	}
	else {
		iter = std::find(m_socksToAdd.begin(), m_socksToAdd.end(), oldSocket);
		assert(iter != m_socksToAdd.end());
		*iter = newSocket;
	}
}

void Poller::run()
{
	m_sockRefs = std::move(m_socksToAdd);
	
    while (!m_sockRefs.empty()) {

		poll();

		if (m_removeHappened) {
			vector<Socket*> socksToPreserve;
			for (auto iter = m_sockRefs.begin(); iter != m_sockRefs.end(); ++iter) {
				if (*iter) {
					socksToPreserve.push_back(*iter);
				}
			}
			m_sockRefs = std::move(socksToPreserve);
		}

		if (!m_socksToAdd.empty()) {
			m_sockRefs.insert(m_sockRefs.end(), m_socksToAdd.begin(), m_socksToAdd.end());
			m_socksToAdd.clear();
		}
    }
}

void Poller::poll()
{
	for (auto iter = m_sockRefs.begin(); iter != m_sockRefs.end(); ++iter) {
		if ((*iter)->waitToRead()) {
			(*iter)->doRead();
		}
		if ((*iter)->waitToWrite()) {
			(*iter)->doWrite();
		}
	}
}
