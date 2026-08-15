#include "workerPool.h"

#include <algorithm>

namespace BreadEngine {
    constexpr unsigned int MAX_WORKERS = 4;

    std::vector<std::thread> WorkerPool::_workers{};
    std::queue<std::packaged_task<void()> > WorkerPool::_jobs{};
    std::mutex WorkerPool::_mutex{};
    std::condition_variable WorkerPool::_wakeup{};
    bool WorkerPool::_stopping = false;

    std::future<void> WorkerPool::submit(std::function<void()> job)
    {
        std::packaged_task<void()> task(std::move(job));
        auto completion = task.get_future();

        {
            std::lock_guard lock(_mutex);
            startWorkers();
            _jobs.push(std::move(task));
        }

        _wakeup.notify_one();
        return completion;
    }

    void WorkerPool::startWorkers()
    {
        if (!_workers.empty()) return;

        _stopping = false;
        const auto available = std::max(1u, std::thread::hardware_concurrency());
        for (auto i = 0u; i < std::min(available, MAX_WORKERS); i++)
        {
            _workers.emplace_back(run);
        }
    }

    void WorkerPool::run()
    {
        while (true)
        {
            std::packaged_task<void()> task;
            {
                std::unique_lock lock(_mutex);
                _wakeup.wait(lock, [] { return _stopping || !_jobs.empty(); });
                // Queued jobs are run even while stopping: a caller may already be waiting on
                // the future, and an unrun packaged_task breaks its promise instead.
                if (_jobs.empty()) return;

                task = std::move(_jobs.front());
                _jobs.pop();
            }

            task();
        }
    }

    void WorkerPool::shutdown()
    {
        {
            std::lock_guard lock(_mutex);
            _stopping = true;
        }

        _wakeup.notify_all();
        for (auto &worker: _workers)
        {
            worker.join();
        }

        _workers.clear();
    }
} // namespace BreadEngine
