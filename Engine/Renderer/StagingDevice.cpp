#include <Renderer/Device.hpp>
#include <Renderer/Renderer.hpp>
#include <Renderer/CommandPool.hpp>
#include <ktx.h>

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
    VK_VERIFY( vkCreateFence( Device::Instance()->GetDevice(), &fci, nullptr, &mStagingFence ) );
}

void Rhi::StagingDevice::Destroy( void ) {
    vkDestroyFence( Device::Instance()->GetDevice(), mStagingFence, nullptr );
}

void Rhi::StagingDevice::Upload( Util::BufferHandle handle, const void * data, size_t size ) {
    BufferMetadata * staging = Device::Instance()->GetBufferPool()->GetMetadata( mStagingBuffer );
    Buffer * buf = Device::Instance()->GetBufferPool()->Get( handle );

    memcpy( static_cast<_byte *>( staging->ptr ), data, size );
    assert( size < mStagingBufferCapacity );

    VkPipelineStageFlags2 dstFlags = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    if ( buf->usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT ) dstFlags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    if ( buf->usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT ) dstFlags |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
    if ( buf->usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ) dstFlags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;

    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();
        VkBufferCopy copy = { .srcOffset = mCurrentOffset, .dstOffset = 0, .size = size };
        cmdlist->Copy( mStagingBuffer, handle, &copy );
        cmdlist->BufferBarrier( handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, dstFlags );
    CommandPool::Instance()->Submit( cmdlist );

    vkWaitForFences( Device::Instance()->GetDevice(), 1, &cmdlist->mFence, VK_TRUE, UINT64_MAX );
    // vkResetFences( Device::Instance()->GetDevice(), 1, &cmdlist->mFence );
}

void Rhi::StagingDevice::Upload( Util::TextureHandle handle, const void * data ) {
    BufferMetadata * staging = Device::Instance()->GetBufferPool()->GetMetadata( mStagingBuffer );
    Texture * tex = Device::Instance()->GetTexturePool()->Get( handle );

    const uint size = tex->extent.width * tex->extent.height * 4;
    memcpy( static_cast<_byte *>( staging->ptr ), data, size );
    assert( size < mStagingBufferCapacity );

    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();
        cmdlist->ImageBarrier( tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
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
        cmdlist->ImageBarrier( tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
    CommandPool::Instance()->Submit( cmdlist );

    vkWaitForFences( Device::Instance()->GetDevice(), 1, &cmdlist->mFence, VK_TRUE, UINT64_MAX );
    // vkResetFences( Device::Instance()->GetDevice(), 1, &cmdlist->mFence );
}

void Rhi::StagingDevice::Upload( Util::TextureHandle handle, ktxTexture2 * ktx ) {
    BufferMetadata * staging = Device::Instance()->GetBufferPool()->GetMetadata( mStagingBuffer );
    Texture * tex = Device::Instance()->GetTexturePool()->Get( handle );

    ktx_size_t stagingOffset = 0, mipOffset;
    vector<ktx_size_t> stagingMipOffsets( ktx->numLevels );
    for ( uint i = 0; i < ktx->numLevels; ++i ) {
        ktxTexture2_GetImageOffset( ktx, i, 0, 0, &mipOffset );

        const ktx_size_t mipsize = ktxTexture_GetLevelSize( ktxTexture(ktx), i );

        _byte * dst = ktxTexture_GetData( ktxTexture(ktx) ) + mipOffset;
        memcpy( static_cast<_byte*>( staging->ptr ) + stagingOffset, dst, mipsize );

        stagingMipOffsets[i] = stagingOffset;
        stagingOffset += mipsize;
    }

    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();
        cmdlist->ImageBarrier( tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
        for( uint i = 0; i < ktx->numLevels; ++i ) {
            const uint mipw = std::max( 1u, ktx->baseWidth >> i );
            const uint miph = std::max( 1u, ktx->baseHeight >> i );

            VkBufferImageCopy copy = {
                .bufferOffset      = stagingMipOffsets[i],
                .bufferRowLength   = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel       = i,
                    .baseArrayLayer = 0,
                    .layerCount     = 1
                },
                .imageOffset = { 0, 0, 0 },
                .imageExtent = { mipw, miph, 1 }
            };
            cmdlist->Copy( mStagingBuffer, tex->image, &copy );
        }
        cmdlist->ImageBarrier( tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
    CommandPool::Instance()->Submit( cmdlist );

    vkWaitForFences( Device::Instance()->GetDevice(), 1, &cmdlist->mFence, VK_TRUE, UINT64_MAX );
}
