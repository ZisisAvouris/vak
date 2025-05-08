#include <Renderer/CommandPool.hpp>
#include <Renderer/Device.hpp>
#include <Renderer/Swapchain.hpp>

void Rhi::CommandPool::Init( void ) {
    const VkCommandPoolCreateInfo ci = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = Device::Instance()->GetQueueIndex( QueueType_Graphics )
    };
    assert( vkCreateCommandPool( Device::Instance()->GetDevice(), &ci, nullptr, &mCommandPool ) == VK_SUCCESS );

    const VkCommandBufferAllocateInfo ai = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = mCommandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    assert( vkAllocateCommandBuffers( Device::Instance()->GetDevice(), &ai, &mCommandList.mBuf ) == VK_SUCCESS );
}

void Rhi::CommandPool::Destroy( void ) {
    vkFreeCommandBuffers( Device::Instance()->GetDevice(), mCommandPool, 1, &mCommandList.mBuf );
    vkDestroyCommandPool( Device::Instance()->GetDevice(), mCommandPool, nullptr );
}

Rhi::CommandList * Rhi::CommandPool::AcquireCommandList( void ) {
    CommandList * list = &mCommandList;
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    assert( vkBeginCommandBuffer( list->mBuf, &beginInfo ) == VK_SUCCESS );
    return list;
}

void Rhi::CommandPool::Submit( CommandList * list, VkImage image ) {
    list->ImageBarrier( image, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT }, { VK_PIPELINE_STAGE_NONE, 0 },
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );

    assert( vkEndCommandBuffer( list->mBuf ) == VK_SUCCESS );

    VkPipelineStageFlags waitStageMasks[]  = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore waitSemaphore = Swapchain::Instance()->GetAcquireSemaphore();
    VkSemaphore signalSemaphore = Swapchain::Instance()->GetRenderCompleteSemaphore();

    const VkSubmitInfo submitInfo = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &waitSemaphore,
        .pWaitDstStageMask    = waitStageMasks,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &list->mBuf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &signalSemaphore,
    };
    VkFence renderFence = Swapchain::Instance()->GetRenderFence();
    assert( vkQueueSubmit( Device::Instance()->GetQueue( QueueType_Graphics ), 1, &submitInfo, renderFence ) == VK_SUCCESS );
}

void Rhi::CommandPool::Submit( CommandList * list, VkFence fence ) {
    assert( vkEndCommandBuffer( list->mBuf ) == VK_SUCCESS );

    const VkSubmitInfo submitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &list->mBuf
    };
    assert( vkQueueSubmit( Device::Instance()->GetQueue( QueueType_Graphics ), 1, &submitInfo, fence ) == VK_SUCCESS );
}
