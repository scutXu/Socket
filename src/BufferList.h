#pragma once

#include <list>

using std::list;

class BufferList
{
public:
	struct Visitor {
		void * ptr;
		unsigned int size;
	};

	void append(unsigned int size);
	unsigned int available();
	unsigned int size();
	unsigned int blockSize();
private:
	list<void *> m_buffers;
	unsigned int m_blockSize;
	unsigned int m_size;

};