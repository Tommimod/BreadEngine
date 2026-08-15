#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace BreadEngine {
    /**
     * Fixed set of worker threads for background asset work.
     *
     * The alternative - a thread per job - spawns one thread per texture the moment a scene
     * loads, and each of them holds a full-size decode buffer at once. A bounded pool caps
     * both the threads and the peak memory, at the cost of decoding in batches.
     */
    class WorkerPool
    {
    public:
        /// Queues @p job on a worker. Wait on the returned future to observe its completion;
        /// letting the future go without waiting does not cancel or wait for the job.
        static std::future<void> submit(std::function<void()> job);

        /// Runs the jobs still queued, then joins the workers.
        static void shutdown();

    private:
        static std::vector<std::thread> _workers;
        static std::queue<std::packaged_task<void()> > _jobs;
        static std::mutex _mutex;
        static std::condition_variable _wakeup;
        static bool _stopping;

        /// Caller must hold _mutex.
        static void startWorkers();

        static void run();
    };
} // namespace BreadEngine
