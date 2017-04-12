//
//  Simple fixed size ring buffer.
//  Thread safe for single Producer and single Consumer.
//  Based on RoingBuffer by Dennis Lang http://home.comcast.net/~lang.dennis/code/ring/ring.html
//  modified to allow in queue use of the blocks without copying them, or doing memory allocation

#pragma once

#include "Block.h"

#include <stddef.h>
class PlannerQueue
{
public:
    PlannerQueue(size_t length)
    {
        m_size= length;
        m_buffer= new Block[length];
        m_rIndex= 0;
        m_wIndex= 0;
    }

    ~PlannerQueue()
    {
        delete [] m_buffer;
    }

    size_t next(size_t n) const
    {
        return (n + 1) % m_size;
    }

    bool empty() const
    {
        return (m_rIndex == m_wIndex);
    }

    bool full() const
    {
        return (next(m_wIndex) == m_rIndex);
    }

    // returns a pointer to the block at the head of the queue (always a new block)
    // this always succeeds as there is always a free block available
    Block* get_head()
    {
        return &m_buffer[m_wIndex];
    }

    // commits the head block to the queue ready for fetching
    // if the queue is full then return false
    bool queue_head()
    {
        if (full())
            return false;

        m_wIndex = next(m_wIndex);
        return true;
    }

    // returns a pointer to the tail of the queue, but does not remove it
    // return nullptr if there is nothing on the queue
    Block* get_tail()
    {
        if (empty())
            return nullptr;
        return &m_buffer[m_rIndex];
    }

    // this releases the tail of the queue to be used again
    void release_tail()
    {
        if(empty()) return; // this should not happen as we should never call this if we did not get a valid tail
        m_rIndex = next(m_rIndex);
    }


private:
    size_t          m_size;
    Block          *m_buffer;

    // volatile is only used to keep compiler from placing values in registers.
    // volatile does NOT make the index thread safe.
    volatile size_t m_rIndex;
    volatile size_t m_wIndex;
};
