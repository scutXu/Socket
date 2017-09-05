#ifndef Poller_h
#define Poller_h

#include <functional>
#include <vector>

using std::function;
using std::vector;

class Socket;

class Poller
{
public:
    Poller();
    virtual ~Poller();
    void add(Socket * socket);
    void remove(Socket * socket);
	void replace(Socket * newSocket, Socket * oldSocket);
    void run();
	void setLoopCallback(const function<void()> & cb);
protected:
	virtual void poll();
private:
    vector<Socket *> m_sockRefs;
	vector<Socket *> m_socksToAdd;
	bool m_removeHappened;

	function<void()> m_loopCallback;
};

#endif /* Poller_h */
