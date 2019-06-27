//
// Created by root on 19-6-27.
//

#ifndef UTIL_COUNTDOWNLATCH_H
#define UTIL_COUNTDOWNLATCH_H

#include <mutex>
#include <condition_variable>

namespace util
{

class CountDownLatch
{
public:
    explicit CountDownLatch(int count);

    void wait();

    void countDown();

    int getCount() const;

private:
    int m_count;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
};

}
#endif //UTIL_COUNTDOWNLATCH_H
