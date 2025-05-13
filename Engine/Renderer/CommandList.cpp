#include <Renderer/CommandPool.hpp>
#include <Renderer/Descriptors.hpp>
#include <Renderer/Pipeline.hpp>
#include <Renderer/Device.hpp>
#include <Renderer/RenderStatistics.hpp>

void Rhi::CommandList::BeginRendering( Util::TextureHandle fbHandle, Util::TextureHandle dbHandle ) {
    Texture * fb = Device::Instance()->GetTexturePool()->Get( fbHandle );
    assert( fb );

    ImageBarrier( fb, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
    VkRenderingAttachmentInfo colorAttachment = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = fb->view,
        .imageLayout = fb->layout,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  = { .color = { 0.0f, 0.0f, 0.0f, 1.0f } }
    };

    VkRenderingAttachmentInfo depthAttachment = {};
    if ( dbHandle.Valid() ) {
        Texture * db = Device::Instance()->GetTexturePool()->Get( dbHandle );
        ImageBarrier( db, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL );
        depthAttachment = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = db->view,
            .imageLayout = db->layout,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = { .depthStencil = { .depth = 0.0f } }
        };
    }

    const VkRect2D renderArea = { { 0, 0 }, { fb->extent.width, fb->extent.height } };
    VkRenderingInfoKHR renderInfo = {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea           = renderArea,
        .layerCount           = 1,
        .viewMask             = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachment,
        .pDepthAttachment     = dbHandle.Valid() ? &depthAttachment : nullptr
    };

    vkCmdSetScissor( mBuf, 0, 1, &renderArea );
    const VkViewport viewport = { 0.0f, 0.0f, (float)fb->extent.width, (float)fb->extent.height, 0.0f, 1.0f };
    vkCmdSetViewport( mBuf, 0, 1, &viewport );
    vkCmdBeginRendering( mBuf, &renderInfo );
}

void Rhi::CommandList::EndRendering( void ) {
    vkCmdEndRendering( mBuf );
}

void Rhi::CommandList::Draw( uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance ) {
    RenderStats::Instance()->cpuDrawCalls++;
    vkCmdDraw( mBuf, vertexCount, instanceCount, firstVertex, firstInstance );
}

void Rhi::CommandList::DrawIndexed( uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance ) {
    RenderStats::Instance()->cpuDrawCalls++;
    vkCmdDrawIndexed( mBuf, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
}

void Rhi::CommandList::DrawIndexedIndirect( Util::BufferHandle indirectBuffer, uint drawCount ) {
    Buffer * buf = Device::Instance()->GetBufferPool()->Get( indirectBuffer );
    RenderStats::Instance()->cpuDrawCalls++;
    RenderStats::Instance()->indirectDrawCalls += drawCount;
    vkCmdDrawIndexedIndirect( mBuf, buf->buf, 0, drawCount, sizeof( VkDrawIndexedIndirectCommand ) );
}

void Rhi::CommandList::BindRenderPipeline( Util::RenderPipelineHandle handle ) {
    RenderPipeline * rp = PipelineFactory::Instance()->GetRenderPipelinePool()->Get( handle );
    if ( !rp ) assert( false );

    vkCmdBindPipeline( mBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, rp->pipeline );

    VkDescriptorSet dset = Descriptors::Instance()->GetDescriptorSet();
    vkCmdBindDescriptorSets( mBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, rp->layout, 0, 1, &dset, 0, nullptr );

    mBoundRP = rp;
}

void Rhi::CommandList::BindVertexBuffer( Util::BufferHandle handle ) {
    Buffer * buf = Device::Instance()->GetBufferPool()->Get( handle );
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers( mBuf, 0, 1, &buf->buf, offsets );
}

void Rhi::CommandList::BindIndexBuffer( Util::BufferHandle handle, VkIndexType indexType ) {
    Buffer * buf = Device::Instance()->GetBufferPool()->Get( handle );
    vkCmdBindIndexBuffer( mBuf, buf->buf, 0, indexType );
}

void Rhi::CommandList::Copy( Util::BufferHandle stagingBuf, Util::BufferHandle dstBuf, VkBufferCopy * copy ) {
    Buffer * staging = Device::Instance()->GetBufferPool()->Get( stagingBuf );
    Buffer * dst     = Device::Instance()->GetBufferPool()->Get( dstBuf );
    vkCmdCopyBuffer( mBuf, staging->buf, dst->buf, 1, copy );
}

void Rhi::CommandList::Copy( Util::BufferHandle stagingBuf, VkImage image, VkBufferImageCopy * copy ) {
    Buffer * staging = Device::Instance()->GetBufferPool()->Get( stagingBuf );
    vkCmdCopyBufferToImage( mBuf, staging->buf, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, copy );
}

void Rhi::CommandList::BufferBarrier( Util::BufferHandle handle, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage ) {
    Buffer * buf = Device::Instance()->GetBufferPool()->Get( handle );
    VkBufferMemoryBarrier2 barrier = {
        .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .srcStageMask        = srcStage,
        .srcAccessMask       = 0,
        .dstStageMask        = dstStage,
        .dstAccessMask       = 0,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer              = buf->buf,
        .offset              = 0,
        .size                = buf->size
    };
    if (srcStage & VK_PIPELINE_STAGE_2_TRANSFER_BIT) {
        barrier.srcAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
    } else {
        barrier.srcAccessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    }

    if (dstStage & VK_PIPELINE_STAGE_2_TRANSFER_BIT) {
        barrier.dstAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
    } else {
        barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    }
    if (dstStage & VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT) {
        barrier.dstAccessMask |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    }
    if (buf->usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
        barrier.dstAccessMask |= VK_ACCESS_2_INDEX_READ_BIT;
    }
    const VkDependencyInfo depInfo = {
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers    = &barrier
    };
    vkCmdPipelineBarrier2( mBuf, &depInfo );
}

void Rhi::CommandList::ImageBarrier( Texture * tex, VkImageLayout newLayout ) {
    const auto [srcStage, srcAccess] = GetLayoutFlags( tex->layout );
    const auto [dstStage, dstAccess] = GetLayoutFlags( newLayout );

    VkImageAspectFlags aspect = isDepthFormat( tex->format ) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageSubresourceRange range = {
        .aspectMask = aspect,
        .levelCount = 1,
        .layerCount = 1,
    };

    const VkImageMemoryBarrier2 barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask        = srcStage,
        .srcAccessMask       = srcAccess,
        .dstStageMask        = dstStage,
        .dstAccessMask       = dstAccess,
        .oldLayout           = tex->layout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = tex->image,
        .subresourceRange    = range,
    };
    const VkDependencyInfo depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier
    };
    vkCmdPipelineBarrier2( mBuf, &depInfo );
    tex->layout = newLayout;
}

void Rhi::CommandList::PushConstants( const void * data, uint size ) {
    assert( mBoundRP );
    vkCmdPushConstants( mBuf, mBoundRP->layout, mBoundRP->shaderStage, 0, size, data );
}

void Rhi::CommandList::BeginDebugLabel( const char * name, const float (& color)[4] ) {
    const VkDebugUtilsLabelEXT label = {
        .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = name,
        .color      = { color[0], color[1], color[2], color[3] }
    };
    vkCmdBeginDebugUtilsLabelEXT( mBuf, &label );
}

void Rhi::CommandList::EndDebugLabel() {
    vkCmdEndDebugUtilsLabelEXT( mBuf );
}
