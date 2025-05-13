#include <Renderer/CommandPool.hpp>
#include <Renderer/Device.hpp>
#include <Renderer/Swapchain.hpp>
#include <Renderer/Timeline.hpp>

void Rhi::CommandPool::Init( void ) {
    const VkCommandPoolCreateInfo ci = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = Device::Instance()->GetQueueIndex( QueueType_Graphics )
    };
    assert( vkCreateCommandPool( Device::Instance()->GetDevice(), &ci, nullptr, &mCommandPool ) == VK_SUCCESS );

    for ( uint i = 0; i < sMaxCommandLists; ++i ) {
        const VkCommandBufferAllocateInfo ai = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = mCommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };
        assert( vkAllocateCommandBuffers( Device::Instance()->GetDevice(), &ai, &mCommandLists[i].mBuf ) == VK_SUCCESS );
        Device::Instance()->RegisterDebugObjectName( VK_OBJECT_TYPE_COMMAND_BUFFER, (ulong)mCommandLists[i].mBuf, "CMDLIST " + std::to_string(i) );

        VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        assert( vkCreateFence( Device::Instance()->GetDevice(), &fci, nullptr, &mCommandLists[i].mFence ) == VK_SUCCESS );
        Device::Instance()->RegisterDebugObjectName( VK_OBJECT_TYPE_FENCE, (ulong)mCommandLists[i].mFence, "CMDLIST Fence " + std::to_string(i) );

        VkSemaphoreCreateInfo sci = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        assert( vkCreateSemaphore( Device::Instance()->GetDevice(), &sci, nullptr, &mCommandLists[i].mSemaphore ) == VK_SUCCESS );
        Device::Instance()->RegisterDebugObjectName( VK_OBJECT_TYPE_SEMAPHORE, (ulong)mCommandLists[i].mSemaphore, "CMDLIST Semaphore " + std::to_string(i) );
    }
}

void Rhi::CommandPool::Destroy( void ) {
    for ( uint i = 0; i < sMaxCommandLists; ++i ) {
        vkFreeCommandBuffers( Device::Instance()->GetDevice(), mCommandPool, 1, &mCommandLists[i].mBuf );
        vkDestroyFence( Device::Instance()->GetDevice(), mCommandLists[i].mFence, nullptr );
        vkDestroySemaphore( Device::Instance()->GetDevice(), mCommandLists[i].mSemaphore, nullptr );
    }
    vkDestroyCommandPool( Device::Instance()->GetDevice(), mCommandPool, nullptr );
}

Rhi::CommandList * Rhi::CommandPool::AcquireCommandList( void ) {
    while ( mCommandListCount == 0 ) {
        Purge();
    }

    CommandList * list = nullptr;
    for ( uint i = 0; i < sMaxCommandLists; ++i ) {
        if ( mCommandLists[i].mReady ) {
            list = &mCommandLists[i];
            list->mReady = false;
            break;
        }
    }
    assert( list );
    --mCommandListCount;

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    assert( vkBeginCommandBuffer( list->mBuf, &beginInfo ) == VK_SUCCESS );
    return list;
}

void Rhi::CommandPool::Submit( CommandList * list, Util::TextureHandle fb ) {
    uint signalSemaphoreCount = 1;
    VkSemaphoreSubmitInfo semaphoresToSignal[2] = {
        { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .semaphore = list->mSemaphore, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },
        { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT }
    };
    uint waitSemaphoreCount = 0;
    VkSemaphoreSubmitInfo semaphoresToWait[2] = {
        { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT },
        { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT }
    };
    if ( mLastSubmitSemaphore ) semaphoresToWait[waitSemaphoreCount++].semaphore = mLastSubmitSemaphore;
    if ( mSwapchainAcquireSemaphore ) semaphoresToWait[waitSemaphoreCount++].semaphore = mSwapchainAcquireSemaphore;

    if ( fb.Valid() ) {
        list->ImageBarrier( Device::Instance()->GetTexturePool()->Get( fb ), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );

        const ulong nextFrameSignalValue = Timeline::Instance()->GetCurrentFrame() + Swapchain::Instance()->GetImageCount();
        Swapchain::Instance()->SetWaitValue( nextFrameSignalValue );

        VkSemaphore timeline = Timeline::Instance()->GetTimeline();
        semaphoresToSignal[signalSemaphoreCount].semaphore = timeline;
        semaphoresToSignal[signalSemaphoreCount].value     = nextFrameSignalValue;
        signalSemaphoreCount++;
    }
    assert( vkEndCommandBuffer( list->mBuf ) == VK_SUCCESS );

    const VkCommandBufferSubmitInfo bufSI = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = list->mBuf };
    const VkSubmitInfo2 submitInfo = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount   = waitSemaphoreCount,
        .pWaitSemaphoreInfos      = semaphoresToWait,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &bufSI,
        .signalSemaphoreInfoCount = signalSemaphoreCount,
        .pSignalSemaphoreInfos    = semaphoresToSignal
    };
    VkResult result = vkQueueSubmit2( Device::Instance()->GetQueue( QueueType_Graphics ), 1, &submitInfo, list->mFence );
    if ( result != VK_SUCCESS ) printf("Queue submit return value: %d\n", result);
    assert( result  == VK_SUCCESS );

    mLastSubmitSemaphore       = list->mSemaphore;
    mSwapchainAcquireSemaphore = VK_NULL_HANDLE;
    mTimelineSemaphore         = VK_NULL_HANDLE;

    if ( fb.Valid() )
        Swapchain::Instance()->Present( std::exchange( mLastSubmitSemaphore, VK_NULL_HANDLE ) );
}

void Rhi::CommandPool::WaitAll( void ) {
    VkFence allFences[sMaxCommandLists];
    uint unsignaledFenceCount = 0;

    for ( uint i = 0; i < sMaxCommandLists; ++i ) {
        if ( mCommandLists[i].mReady == false ) {
            allFences[unsignaledFenceCount] = mCommandLists[i].mFence;
            unsignaledFenceCount++;
        }
    }
    assert( vkWaitForFences( Device::Instance()->GetDevice(), unsignaledFenceCount, allFences, VK_TRUE, UINT64_MAX ) == VK_SUCCESS );
}

void Rhi::CommandPool::Purge( void ) {
    for ( uint i = 0; i < sMaxCommandLists; ++i ) {
        if ( vkGetFenceStatus( Device::Instance()->GetDevice(), mCommandLists[i].mFence ) == VK_SUCCESS ) {
            assert( !mCommandLists[i].mReady );
            assert( vkResetCommandBuffer( mCommandLists[i].mBuf, 0 ) == VK_SUCCESS );
            assert( vkResetFences( Device::Instance()->GetDevice(), 1, &mCommandLists[i].mFence ) == VK_SUCCESS );
            mCommandLists[i].mReady = true;
            ++mCommandListCount;
        }
    }
}
