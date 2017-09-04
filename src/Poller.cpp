#include "Poller.h"
#include "Socket.h"

void Poller::add(Socket *socket)
{
    m_sockRefs.push_back(socket);
}

void Poller::remove(Socket *socket)
{
    auto iter = std::find(m_sockRefs.begin(), m_sockRefs.end(), socket);
    if(iter != m_sockRefs.end()) {
        m_sockRefs.erase(iter);
    }
}

void Poller::replace(Socket *newSocket, Socket *oldSocket)
{
    auto iter = std::find(m_sockRefs.begin(), m_sockRefs.end(), oldSocket);
    *iter = newSocket;
}

void Poller::poll()
{
    while (!m_sockRefs.empty()) {
        for (auto iter = m_sockRefs.begin(); iter != m_sockRefs.end(); ++iter) {
            if ((*iter)->waitToRead()) {
                (*iter)->doRead();
            }
            if ((*iter)->waitToWrite()) {
                (*iter)->doWrite();
            }
        }
    }
}
