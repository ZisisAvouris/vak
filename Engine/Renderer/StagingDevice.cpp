#include <Renderer/Device.hpp>
#include <Renderer/Renderer.hpp>
#include <Renderer/CommandPool.hpp>

void Rhi::StagingDevice::Init( void ) {
    mStagingBufferCapacity = 512 * 1024 * 1024; // @todo: need to check against device limits
    mStagingBuffer = Device::Instance()->CreateBuffer({
        .usage     = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .storage   = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .size      = mStagingBufferCapacity,
        .debugName = "Staging Device"
    });
    mStagingBufferSize = 0;
    mCurrentOffset     = 0;

    VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    assert( vkCreateFence( Device::Instance()->GetDevice(), &fci, nullptr, &mStagingFence ) == VK_SUCCESS );
}

void Rhi::StagingDevice::Destroy( void ) {
    vkDestroyFence( Device::Instance()->GetDevice(), mStagingFence, nullptr );
}

void Rhi::StagingDevice::Upload( Util::BufferHandle handle, const void * data, size_t size ) {
    BufferMetadata * staging = Device::Instance()->GetBufferPool()->GetMetadata( mStagingBuffer );

    memcpy( static_cast<_byte *>( staging->ptr ) + mCurrentOffset, data, size );
    assert( mCurrentOffset < mStagingBufferCapacity );
    
    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();
        VkBufferCopy copy = { .srcOffset = mCurrentOffset, .dstOffset = 0, .size = size };
        cmdlist->Copy( mStagingBuffer, handle, &copy );
        cmdlist->BufferBarrier( handle );
    CommandPool::Instance()->Submit( cmdlist, mStagingFence );

    mCurrentOffset += size;
    
    vkWaitForFences( Device::Instance()->GetDevice(), 1, &mStagingFence, VK_TRUE, UINT64_MAX );
    vkResetFences( Device::Instance()->GetDevice(), 1, &mStagingFence );
}

void Rhi::StagingDevice::Upload( Util::TextureHandle handle, const void * data ) {
    BufferMetadata * staging = Device::Instance()->GetBufferPool()->GetMetadata( mStagingBuffer );
    Texture * tex = Device::Instance()->GetTexturePool()->Get( handle );

    const uint size = tex->extent.width * tex->extent.height * 4;
    memcpy( static_cast<_byte *>( staging->ptr ) + mCurrentOffset, data, size );
    assert( mCurrentOffset < mStagingBufferCapacity );

    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();
        cmdlist->ImageBarrier( tex->image, { .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
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
            .imageExtent = tex->extent
        };
        cmdlist->Copy( mStagingBuffer, tex->image, &copy );
        cmdlist->ImageBarrier( tex->image, { .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
            { .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
    CommandPool::Instance()->Submit( cmdlist, mStagingFence );

    mCurrentOffset += size;

    vkWaitForFences( Device::Instance()->GetDevice(), 1, &mStagingFence, VK_TRUE, UINT64_MAX );
    vkResetFences( Device::Instance()->GetDevice(), 1, &mStagingFence );
}
