#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>

namespace aisdk::base {

template <typename T>
class SharedQueue {
   public:
    SharedQueue() = default;

    SharedQueue &operator=(const SharedQueue &) = delete;
    SharedQueue(const SharedQueue &other) = delete;

    void open() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_run_out = false;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_run_out = true;
        }
        m_data_cond.notify_all();
    }

    void push(T &&item) {
        if (!m_run_out) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_queue.push(std::move(item));
            }
            m_data_cond.notify_one();
        }
    }

    bool try_and_pop(T &popped_item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_run_out || m_queue.empty()) {
            return false;
        }

        popped_item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool wait_and_pop(T &popped_item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty() && !m_run_out) {
            m_data_cond.wait(lock);
        }

        if (m_run_out || m_queue.empty()) {
            return false;
        }

        popped_item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool wait_for_and_pop(T &popped_item, uint32_t timeout_ms) {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty() && !m_run_out) {
            m_data_cond.wait_for(lock, std::chrono::milliseconds(timeout_ms));
        }

        if (m_run_out || m_queue.empty()) {
            return false;
        }

        popped_item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    unsigned size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) {
            m_queue.pop();
        }
    }

   private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_data_cond;
    bool m_run_out = false;
};

}  // namespace aisdk::base