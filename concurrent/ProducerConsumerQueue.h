//
// Created by zibo on 19-7-6.
//

#ifndef UTIL_PRODUCERCONSUMERQUEUE_H
#define UTIL_PRODUCERCONSUMERQUEUE_H

#include <new>
#include <atomic>
#include <utility>
#include <type_traits>
#include <stdexcept>
#include <thread>
#include <cassert>

template <typename T>
class ProducerConsumerQueue
{
public:
    using value_type = T;

    ProducerConsumerQueue(const ProducerConsumerQueue&) = delete;
    ProducerConsumerQueue&operator= (const ProducerConsumerQueue&) = delete;

    explicit ProducerConsumerQueue(uint32_t size)
        :size_{size},
        records_{static_cast<T*>(std::malloc(sizeof(T) * size))},
        readIndex_{0},
        writeIndex_{0}
    {
        assert(size >= 2);
        if (!records_)
        {
            throw std::bad_alloc();
        }
    }

    ~ProducerConsumerQueue()
    {
        if (!std::is_trivially_destructible<T>::value)
        {
            size_t readIndex = readIndex_;
            size_t endIndex = writeIndex_;

            while (readIndex != endIndex)
            {
                records_[readIndex].~T();
                if (++readIndex == size_)
                {
                    readIndex = 0;
                }
            }
        }

        std::free(records_);
    }

    //Producer thread
    template <typename... Args>
    bool write(Args&&... recordArgs)
    {
        auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
        auto nextRecord = currentWrite + 1;
        if (nextRecord == size_)
        {
            nextRecord = 0;
        }

        if (nextRecord != readIndex_.load(std::memory_order_acquire))
        {
            new (&records_[currentWrite]) T(std::forward<Args>(recordArgs)...);
            writeIndex_.store(nextRecord, std::memory_order_release);

            return true;
        }

        return false;
    }

    //Consumer thread
    bool read(T& record)
    {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        if (currentRead == writeIndex_.load(std::memory_order_acquire))
        {
            return false;
        }

        auto nextRecord = currentRead + 1;
        if (nextRecord == size_)
        {
            nextRecord = 0;
        }

        record = std::move(records_[currentRead]);
        records_[currentRead].~T();
        readIndex_.store(nextRecord, std::memory_order_release);

        return true;
    }

    //Consumer thread
    T* frontPtr()
    {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        if (currentRead == writeIndex_.load(std::memory_order_acquire))
        {
            return nullptr;
        }

        return &records_[currentRead];
    }

    //Consumer thread
    //queue must not be empty
    void popFront()
    {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        assert(currentRead != writeIndex_.load(std::memory_order_acquire));

        auto nextRecord = currentRead + 1;
        if (nextRecord == size_)
        {
            nextRecord = 0;
        }

        records_[currentRead].~T();
        readIndex_.store(nextRecord, std::memory_order_release);
    }

    bool isEmpty() const
    {
        return readIndex_.load(std::memory_order_acquire) ==
               writeIndex_.load(std::memory_order_acquire);
    }

    bool isFull() const
    {
        auto nextRecord = writeIndex_.load(std::memory_order_acquire) + 1;
        if (nextRecord == size_)
        {
            nextRecord = 0;
        }

        if (nextRecord != readIndex_.load(std::memory_order_acquire))
        {
            return false;
        }

        return true;
    }

    size_t sizeGuess() const
    {
        int ret = writeIndex_.load(std::memory_order_acquire) -
                  readIndex_.load(std::memory_order_acquire);

        if (ret < 0)
        {
            ret += size_;
        }

        return ret;
    }

    size_t capacity() const
    {
        return size_ - 1;
    }

private:
    using AtomicIndex = std::atomic<unsigned int>;
    const static int hardware_destructive_interference_size = 128;

    char pad0_[hardware_destructive_interference_size];
    const uint32_t size_;
    T* const records_;

    alignas(hardware_destructive_interference_size) AtomicIndex readIndex_;
    alignas(hardware_destructive_interference_size) AtomicIndex writeIndex_;

    char pad1_[hardware_destructive_interference_size - sizeof(AtomicIndex)];
};

#endif //UTIL_PRODUCERCONSUMERQUEUE_H
