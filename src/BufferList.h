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

	struct Mark
	{
		list<void *>::iterator iter;
		unsigned int offset;
	};

	BufferList(unsigned int blockSize);
	~BufferList();

	unsigned int getSize();
	unsigned int getBlockSize();
	unsigned int getAvailable();
	unsigned int getCapacity();

	Visitor getHead();
	Visitor getTail();

	void append(unsigned int size);
	void commit(unsigned int size);
	void consume(unsigned int size);
	void clear();
private:
	list<void *> m_buffers;
	Mark m_head;
	Mark m_tail;
	unsigned int m_blockSize;
	unsigned int m_size;
};