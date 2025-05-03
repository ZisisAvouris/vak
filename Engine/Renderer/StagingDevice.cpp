#include <Renderer/Device.hpp>
#include <Renderer/CommandPool.hpp>

void Rhi::StagingDevice::Init( void ) {
    mStagingBufferCapacity = 128 * 1024 * 1024; // @todo: need to check against device limits
    mStagingBuffer = CreateBuffer( VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, mStagingBufferCapacity );
    mStagingBufferSize = 0;
    mCurrentOffset     = 0;

    VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    assert( vkCreateFence( Device::Instance()->GetDevice(), &fci, nullptr, &mStagingFence ) == VK_SUCCESS );
}

void Rhi::StagingDevice::Destroy( void ) {
    vkDestroyFence( Device::Instance()->GetDevice(), mStagingFence, nullptr );
}

void Rhi::StagingDevice::Upload( const Buffer & buf, const void * data, size_t size ) {
    memcpy( static_cast<_byte *>( mStagingBuffer.ptr ) + mCurrentOffset, data, size );
    assert( mCurrentOffset < mStagingBufferCapacity );
    
    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();
        VkBufferCopy copy = { .srcOffset = mCurrentOffset, .dstOffset = 0, .size = size };
        cmdlist->Copy( mStagingBuffer, buf, &copy );
        cmdlist->BufferBarrier( buf );
    CommandPool::Instance()->Submit( cmdlist, mStagingFence );

    mCurrentOffset += size;
    printf("[Staging Device] Current Offset %u out of capacity %u\n", mCurrentOffset, mStagingBufferCapacity );
    
    vkWaitForFences( Device::Instance()->GetDevice(), 1, &mStagingFence, VK_TRUE, UINT64_MAX );
    vkResetFences( Device::Instance()->GetDevice(), 1, &mStagingFence );
}

void Rhi::StagingDevice::Upload( const Texture & tex, const void * data ) {
    const uint size = tex.extent.width * tex.extent.height * 4;
    memcpy( static_cast<_byte *>( mStagingBuffer.ptr ) + mCurrentOffset, data, size );
    assert( mCurrentOffset < mStagingBufferCapacity );

    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();
        cmdlist->ImageBarrier( tex.image, { .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
            { .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
        VkBufferImageCopy copy = {
            .bufferOffset      = mCurrentOffset,
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = 0,
                .baseArrayLayer = 0,
                .layerCount     = 1
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = tex.extent
        };
        cmdlist->Copy( mStagingBuffer, tex, &copy );
        cmdlist->ImageBarrier( tex.image, { .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
            { .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
    CommandPool::Instance()->Submit( cmdlist, mStagingFence );

    mCurrentOffset += size;
    printf("[Staging Device] Current Offset %u out of capacity %u\n", mCurrentOffset, mStagingBufferCapacity );

    vkWaitForFences( Device::Instance()->GetDevice(), 1, &mStagingFence, VK_TRUE, UINT64_MAX );
    vkResetFences( Device::Instance()->GetDevice(), 1, &mStagingFence );
}
