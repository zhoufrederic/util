// Use of this source code is governed by a MIT-style license
// that can be found in the License file.
//
//
// Created by zibo on 19-6-18.
//

#ifndef UTIL_ARRAYBLOCKINGQUEUE_H
#define UTIL_ARRAYBLOCKINGQUEUE_H

#include <vector>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <iostream>

namespace util
{

    template <typename T>
    class ArrayBlockingQueue
    {
    public:
        explicit ArrayBlockingQueue(int maxSize) : m_takeIndex{0},
                                                   m_putIndex{0},
                                                   m_count{0},
                                                   m_length{maxSize}
        {
            m_queue = (T*)malloc(maxSize * sizeof(T));
        }

        ~ArrayBlockingQueue()
        {
            free(m_queue);
        }

        bool add(T t)
        {
            std::unique_lock<std::mutex> uniqueLock{m_mutex};
            if (m_count == m_length)
            {
                return false;
            }
            else
            {
                enqueue(std::forward<T>(t));
                return true;
            }
        }

        void put(T t)
        {
            std::unique_lock<std::mutex> uniqueLock{m_mutex};
            while (m_count == m_length)
            {
                m_notFull.wait(uniqueLock);
            }
            enqueue(std::forward<T>(t));
        }

        T take()
        {
            std::unique_lock<std::mutex> uniqueLock{m_mutex};
            while (m_count == 0)
            {
                m_notEmpty.wait(uniqueLock);
            }

            return dequeue();
        }

    private:

        //withLockHold
        void enqueue(const T& t)
        {
            m_queue[m_putIndex] = t;

            if (++m_putIndex == this->m_length)
            {
                m_putIndex = 0;
            }

            ++m_count;

            m_notEmpty.notify_one();
        }

        //withLockHold
        void enqueue(T&& t)
        {
            m_queue[m_putIndex] = std::move(t);
            if (++m_putIndex == m_length)
            {
                m_putIndex = 0;
            }

            ++m_count;

            m_notEmpty.notify_one();
        }

        //withLockHold
        T dequeue()
        {
            T t{std::move(m_queue[m_takeIndex])};
            if (++m_takeIndex == m_length)
            {
                m_takeIndex = 0;
            }

            --m_count;

            m_notFull.notify_one();

            return std::move(t);
        }

        //withLockHold
        void removeAt(int removeIndex)
        {
            if (removeIndex == m_takeIndex)
            {
                if (++m_takeIndex == this->m_length)
                {
                    m_takeIndex = 0;
                }
                --m_count;
            }
            else
            {
                for (int i = removeIndex;;)
                {
                    int pred = i;
                    if (++i == this->m_length)
                    {
                        i = 0;
                    }

                    if (i == m_putIndex)
                    {
                        m_putIndex = pred;
                        break;
                    }
                    m_queue[pred] = m_queue[i];
                }
                --m_count;
            }

            m_notFull.notify_one();
        }

    private:
        int m_takeIndex;
        int m_putIndex;
        int m_count;
        int m_length;
        T* m_queue;

        std::mutex m_mutex;
        std::condition_variable m_notEmpty;
        std::condition_variable m_notFull;
    };
}

#endif //UTIL_ARRAYBLOCKINGQUEUE_H
