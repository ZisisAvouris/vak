#include <Renderer/Timeline.hpp>
#include <Renderer/Device.hpp>

void Rhi::Timeline::Init( void ) {
    const VkSemaphoreTypeCreateInfo typeCI = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue  = 0
    };
    const VkSemaphoreCreateInfo semCI = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &typeCI
    };
    assert( vkCreateSemaphore( Device::Instance()->GetDevice(), &semCI, nullptr, &mTimeline ) == VK_SUCCESS );
    Device::Instance()->RegisterDebugObjectName( VK_OBJECT_TYPE_SEMAPHORE, (ulong)mTimeline, "Timeline Semaphore" );
    mFrame = 0;
}

void Rhi::Timeline::Destroy( void ) {
    vkDestroySemaphore( Device::Instance()->GetDevice(), mTimeline, nullptr );
}

void Rhi::Timeline::SignalTimeline( ulong value ) {
    const VkSemaphoreSignalInfo signalInfo = {
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .semaphore = mTimeline,
        .value     = value
    };
    assert( vkSignalSemaphore( Device::Instance()->GetDevice(), &signalInfo ) == VK_SUCCESS );
}
