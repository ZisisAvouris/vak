#include <Core/JobSystem.hpp>

void Core::JobSystem::Init() {
    const uint threads = std::thread::hardware_concurrency();
    mThreadPool.reserve( threads );

    for ( uint i = 0; i < threads; ++i ) {
        mThreadPool.emplace_back( &Core::JobSystem::ThreadMainLoop, this );
    }
    mRunning = true;
}

void Core::JobSystem::Destroy() {
    mRunning = false;
    mWaitCondition.notify_all();

    for ( auto & thread : mThreadPool ) {
        if ( thread.joinable() )
            thread.join();
    }
}

void Core::JobSystem::DispatchJob( Job job, JobPriority priority ) {
    std::lock_guard<std::mutex> lock( mJobQueueMutex );
    switch ( priority ) {
        case JobPriority::High:   mHighQueue.push( job );   break;
        case JobPriority::Normal: mNormalQueue.push( job ); break;
        case JobPriority::Low:    mLowQueue.push( job );    break;
    }
    mJobCounter.fetch_add( 1, std::memory_order_relaxed );
    mWaitCondition.notify_one();
}

void Core::JobSystem::WaitAll() {
    while ( mJobCounter.load( std::memory_order_acquire ) != 0 ) {
        mJobCounter.wait( mJobCounter.load() );
    }
}

void Core::JobSystem::ThreadMainLoop() {
    auto pullJob = [this]() -> Job {
        Job job = nullptr;
        if ( !mHighQueue.empty() ) {
            job = mHighQueue.front();
            mHighQueue.pop();
        } else if ( !mNormalQueue.empty() ) {
            job = mNormalQueue.front();
            mNormalQueue.pop();
        } else if ( !mLowQueue.empty() ) {
            job = mLowQueue.front();
            mLowQueue.pop();
        }
        return job;
    };

    while ( true ) {
        Job job = nullptr;
        {
            std::unique_lock<std::mutex> lock( mJobQueueMutex );
            mWaitCondition.wait( lock, [&] { return !mRunning || !mHighQueue.empty() || !mNormalQueue.empty() || !mLowQueue.empty(); } );

            if ( !mRunning )
                break;
            job = pullJob();
        }

        if ( job ) {
            job();
            if ( mJobCounter.fetch_sub( 1, std::memory_order_acq_rel ) == 1 ) {
                mJobCounter.notify_all();
            }
        }
    }
}


