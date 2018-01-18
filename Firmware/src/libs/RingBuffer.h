//
//  Simple fixed size ring buffer, modified from PlannerQueue header file.
//  Thread safe for single Producer and single Consumer.
//  Based on RoingBuffer by Dennis Lang http://home.comcast.net/~lang.dennis/code/ring/ring.html
//  modified to allow in queue use of the objects without copying them, or doing memory allocation

#pragma once

#include <stddef.h>

class RingBuffer
{
    public:
        /**
         * @brief   Initialize ringbuffer
         * @param   number of elements in buffer
         * @return  Nothing
         * @note    The module is initialized with head and tail indexes
         * pointed to the first element and iter is a index used internally
         */
        RingBuffer(size_t length)
        {
            size=length;
            tail = 0;
            head = 0;
            iter = 0;
            buffer=(float*)malloc(sizeof(int) * size);
        }

        /**
         * @brief   Deinitialize ringbuffer
         * @param   Nothing
         * @return  Nothing
         * @note    Deallocates the buffer from memory
         */
        ~RingBuffer()
        {
            delete [] buffer;
        }

        /**
         * @brief   Get next index of the reference index
         * @param   Reference index
         * @return  Right index of the reference index
         * @note    If reference index is the last of the buffer, returns first index instead
         */
        size_t next(size_t n) const
        {
            return (n + 1) % size;
        }

        /**
         * @brief   Get previous index of the reference index
         * @param   Reference index
         * @return  Left index of the reference index
         * @note    If reference index is the first of the buffer, returns last index instead
         */
        size_t prev(size_t n) const
        {
            if(n == 0) return size - 1;
            return n - 1;
        }

        /**
         * @brief   Check if the buffer is empty
         * @param   Nothing
         * @return  True if the buffer is empty
         * @return  False if the buffer is not empty
         */
        bool empty() const
        {
            return (tail == head);
        }

        /**
         * @brief   Check if the buffer is full
         * @param   Nothing
         * @return  True if the buffer is full
         * @return  False if the buffer is not full
         */
        bool full() const
        {
            return (next(head) == tail);
        }

        /**
         * @brief   Get head element
         * @param   Nothing
         * @return  Pointer to the element at the head of the queue
         */
        float* get_head()
        {
            return &buffer[head];
        }

        /**
         * @brief   Commit the head element to the queue ready for fetching
         * @param   Nothing
         * @return  True if buffer is not full and head index gets to the next index
         * @return  False if the buffer is full
         */
        bool queue_head()
        {
            if (full())
                return false;

            head = next(head);
            return true;
        }

        /**
         * @brief   Get tail element
         * @param   Nothing
         * @return  Pointer to the element at the tail of the queue
         * @return  Null pointer if the buffer is empty
         */
        float* get_tail()
        {
            if (empty())
                return nullptr;
            return &buffer[tail];
        }

        /**
         * @brief   Release tail element from buffer if the buffer is not empty
         * @param   Nothing
         * @return  Nothing
         */
        void release_tail()
        {
            if(empty()) return;
            tail = next(tail);
        }

        /**
         * @brief   Gets element from index previous to internal reference index, moving it to that index
         * @param   Nothing
         * @return  Pointer to the element at the specific index
         */
        float* tailward_get()
        {
            iter = prev(iter);
            return &buffer[iter];
        }

        /**
         * @brief   Gets element from index next to internal reference index, moving it to that index
         * @param   Nothing
         * @return  Pointer to the element at the specific index
         */
        float* headward_get()
        {
            iter = next(iter);
            return &buffer[iter];
        }

        /**
         * @brief   Check if internal reference is pointing to tail element
         * @param   Nothing
         * @return  True if internal reference index and tail index are equal
         * @return  False if internal reference index and tail index are not equal
         */
        bool is_at_tail()
        {
            return iter == tail;
        }

        /**
         * @brief   Check if internal reference is pointing to head element
         * @param   Nothing
         * @return  True if internal reference index and head index are equal
         * @return  False if internal reference index and head index are not equal
         */
        bool is_at_head()
        {
            return iter == head;
        }

        /**
         * @brief   Start iteration with internal reference pointing to the head element
         * @return  Nothing
         * @return  Nothing
         */
        void start_iteration()
        {
            iter = head;
        }

        /**
         * @brief   End iteration with internal reference pointing to the tail element
         * @return  Nothing
         * @return  Nothing
         */
        void end_iteration()
        {
            iter = tail;
        }

        /**
         * @brief   Next iteration with internal reference pointing to the next element
         * @return  Nothing
         * @return  Nothing
         */
        void next_iteration()
        {
            iter = next(iter);
        }

        /**
         * @brief   Previous iteration with internal reference pointing to the previous element
         * @return  Nothing
         * @return  Nothing
         */
        void prev_iteration()
        {
            iter = prev(iter);
        }

        /**
         * @brief   Get actual size of the buffer
         * @param   Nothing
         * @return  The number of elements between the head and tail elements
         */
        size_t get_size()
        {
            return (tail > head ? size : 0) + head - tail;
        }

        /**
         * @brief   Stores object to the head position
         * @param   New float object to be stored in the buffer
         * @return  Nothing
         */
        void push_back(float object)
        {
            buffer[head] = object;
            head = next(head);
        }

        /**
         * @brief   Gets internal reference object
         * @param   Nothing
         * @return  Nothing
         */
        float* get_ref()
        {
            return &buffer[iter];
        }

        /**
         * @brief   Resets the buffer by releasing all elements from the buffer
         * @param   Nothing
         * @return  Nothing
         */
        void reset()
        {
            if(empty()) return;
            tail = head;
        }
    private:
        float *buffer;
        size_t iter;   //Internal pointer
        size_t tail;   //Pointer to the oldest object
        size_t head;   //Pointer to the newest object
        size_t size;   //Fixed size of the buffer
};
