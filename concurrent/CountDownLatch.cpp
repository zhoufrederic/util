//
// Created by root on 19-6-27.
//

#include "CountDownLatch.h"

using namespace util;

CountDownLatch::CountDownLatch(int count) : m_count{count} {}

void CountDownLatch::wait()
{
    std::unique_lock<std::mutex> uniqueLock{m_mutex};
    while (m_count > 0)
    {
        m_condition.wait(uniqueLock);
    }
}

void CountDownLatch::countDown()
{
    std::unique_lock<std::mutex> uniqueLock{m_mutex};
    --m_count;

    if (m_count == 0)
    {
        m_condition.notify_all();
    }
}

int CountDownLatch::getCount() const
{
    std::lock_guard<std::mutex> lockGuard{m_mutex};
    return m_count;
}