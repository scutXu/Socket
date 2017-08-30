#include "BufferList.h"

BufferList::BufferList(unsigned int blockSize)
{
	m_blockSize = blockSize;
	m_head.iter = m_buffers.end();
	m_head.offset = 0;
	m_tail.iter = m_buffers.end();
	m_tail.offset = 0;
}

BufferList::~BufferList()
{
	clear();
}

unsigned int BufferList::getSize()
{
	return m_size;
}

unsigned int BufferList::getBlockSize()
{
	return m_blockSize;
}

unsigned int BufferList::getAvailable()
{
	if (m_tail.iter == m_buffers.end()) {
		return 0;
	}
	int iterOffset = 0;
	list<void *>::iterator tailIter = m_tail.iter;
	while (tailIter != m_buffers.end())
	{
		++tailIter;
		++iterOffset;
	}
	return (m_blockSize - m_tail.offset) + m_blockSize * (iterOffset - 1);
}

unsigned int BufferList::getCapacity()
{
	return m_buffers.size() * m_blockSize;
}

void BufferList::append(unsigned int size)
{

}