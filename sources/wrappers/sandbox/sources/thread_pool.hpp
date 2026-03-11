#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/debug/assert.hpp>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace tavros::core
{
    template<size_t MaxThreads = 64>
    class basic_thread_pool : noncopyable
    {
    public:
        explicit basic_thread_pool(size_t threads_number)
            : m_stop(false)
            , m_active_tasks(0)
        {
            TAV_ASSERT(threads_number <= MaxThreads);
            TAV_ASSERT(threads_number > 0);

            for (size_t i = 0; i < threads_number; ++i) {
                m_workers.emplace_back([this] {
                    for (;;) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lk(m_queue_mt);
                            m_cv.wait(lk, [this] { return m_stop || !m_tasks.empty(); });
                            if (m_stop && m_tasks.empty()) {
                                return;
                            }
                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                        }
                        task();

                        {
                            std::unique_lock<std::mutex> lk(m_queue_mt);
                            --m_active_tasks;
                        }
                        m_idle_cv.notify_all();
                    }
                });
            }
        }

        ~basic_thread_pool() noexcept
        {
            {
                std::unique_lock<std::mutex> lk(m_queue_mt);
                m_stop = true;
            }
            m_cv.notify_all();
            for (auto& worker : m_workers) {
                worker.join();
            }
        }

        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args)
            -> std::future<typename std::invoke_result<F, Args...>::type>
        {
            using return_type = typename std::invoke_result<F, Args...>::type;

            auto task = std::make_shared<std::packaged_task<return_type()>>(
                [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable {
                    return f(std::forward<Args>(args)...);
                }
            );

            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lk(m_queue_mt);
                if (m_stop) {
                    throw std::runtime_error("enqueue on stopped basic_thread_pool");
                }

                ++m_active_tasks;
                m_tasks.emplace([task]() { (*task)(); });
            }
            m_cv.notify_one();
            return res;
        }

        void wait_all()
        {
            std::unique_lock<std::mutex> lk(m_queue_mt);
            m_idle_cv.wait(lk, [this] { return m_active_tasks == 0; });
        }

        size_t worker_count() const noexcept
        {
            return m_workers.size();
        }

        size_t pending_tasks() const
        {
            std::unique_lock<std::mutex> lk(m_queue_mt);
            return m_active_tasks;
        }

    private:
        core::static_vector<std::thread, MaxThreads> m_workers;
        std::queue<std::function<void()>>            m_tasks;

        mutable std::mutex      m_queue_mt;
        std::condition_variable m_cv;
        std::condition_variable m_idle_cv;
        bool                    m_stop;
        size_t                  m_active_tasks;
    };

    using thread_pool = basic_thread_pool<>;

} // namespace tavros::core
