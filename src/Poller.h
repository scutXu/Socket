#ifndef Poller_h
#define Poller_h

#include <vector>

using std::vector;

class Socket;

class Poller
{
public:
    struct SocketReference
    {
        SocketReference(Socket * s)
        {
            socket = s;
            markClosed = false;
        }
        Socket * socket;
        bool markClosed;
    };
    
    void add(Socket * socket);
    void replace(Socket * newSocket, Socket * oldSocket);
    void remove(Socket * socket);
    void poll();
private:
    vector<SocketReference> m_sockRefs;
};

#endif /* Poller_h */
