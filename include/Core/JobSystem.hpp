#pragma once
#include <Util/Defines.hpp>
#include <Util/Singleton.hpp>

#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace Core {

    using Job = std::function<void()>;

    enum class JobPriority : _byte {
        Low    = 1,
        Normal = 2,
        High   = 3
    };

    class JobSystem final : public Core::Singleton<JobSystem> {
    public:
        void Init();
        void Destroy();

        void DispatchJob( Job, JobPriority = JobPriority::High );

        void WaitAll();

    private:
        std::vector<std::thread> mThreadPool;

        std::condition_variable mWaitCondition;
        std::mutex              mJobQueueMutex;

        std::atomic<uint> mJobCounter;

        std::queue<Job> mLowQueue;
        std::queue<Job> mNormalQueue;
        std::queue<Job> mHighQueue;

        bool mRunning;

        void ThreadMainLoop();
    };

}
