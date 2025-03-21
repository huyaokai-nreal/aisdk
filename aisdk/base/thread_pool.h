#pragma once
#include <sys/prctl.h>

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>
namespace aisdk::base {
class ThreadPool {
   public:
    explicit ThreadPool(size_t threads, size_t depth = 10, const std::string& thread_name = "ThreadPool");
    ThreadPool(const ThreadPool&) = delete; // 禁用复制构造函数
    ThreadPool& operator=(const ThreadPool&) = delete; // 禁用赋值构造函数
    ThreadPool(ThreadPool&&) = delete; // 禁用移动构造函数
    ThreadPool& operator=(ThreadPool&&) = delete; // 禁用移动赋值构造函数
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();
    void stop(){
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_ = true;
        condition.notify_all(); // Notify all waiting threads to wake up and check the stop condition
    }

   private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;
    // the task queue
    size_t max_depth;
    std::queue<std::function<void()> > tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop_ = false;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads, size_t depth, const std::string& thread_name) : max_depth(depth){
    for (size_t i = 0; i < threads; ++i) {
        std::string name = thread_name + "-" + std::to_string(i);
        workers.emplace_back([this, name] {
            prctl(PR_SET_NAME, name.c_str(), NULL, NULL, NULL);
            for (;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop_ || !this->tasks.empty(); });
                    if (this->stop_ && this->tasks.empty()) { return;
}
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                task();
            }
        });
    }
}

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if ((max_depth > 0 && tasks.size() > max_depth) || stop_) {
            return res;
        }

        res = task->get_future();

        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_ = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) { worker.join();
}
}

}  // namespace aisdk::base