#include <Renderer/CommandPool.hpp>
#include <Renderer/Descriptors.hpp>

void Rhi::CommandList::BeginRendering( uint2 res, VkImage image, VkImageView view ) {
    ImageBarrier( image, { VK_PIPELINE_STAGE_NONE, 0 }, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL );

    VkRenderingAttachmentInfo colorAttachment = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  = { .color = { 0.0f, 0.0f, 0.0f, 1.0f } }
    };
    VkRect2D renderArea = { {0,0}, {res.x, res.y} };
    VkRenderingInfoKHR renderInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = renderArea,
        .layerCount = 1,
        .viewMask   = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachment
    };

    vkCmdSetScissor( mBuf, 0, 1, &renderArea );
    const VkViewport viewport = { 0.0f, 0.0f, (float)res.x, (float)res.y, 0.0f, 1.0f };
    vkCmdSetViewport( mBuf, 0, 1, &viewport );
    vkCmdBeginRendering( mBuf, &renderInfo );
}

void Rhi::CommandList::EndRendering( void ) {
    vkCmdEndRendering( mBuf );
}

void Rhi::CommandList::Draw( uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance ) {
    vkCmdDraw( mBuf, vertexCount, instanceCount, firstVertex, firstInstance );
}

void Rhi::CommandList::DrawIndexed( uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance ) {
    vkCmdDrawIndexed( mBuf, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
}

void Rhi::CommandList::BindRenderPipeline( const RenderPipeline & rp ) {
    vkCmdBindPipeline( mBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, rp.pipeline );

    VkDescriptorSet dset = Descriptors::Instance()->GetDescriptorSet();
    vkCmdBindDescriptorSets( mBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, rp.layout, 0, 1, &dset, 0, nullptr );

    mBoundRP = &rp;
}

void Rhi::CommandList::BindVertexBuffer( const Buffer & vertexBuffer ) {
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers( mBuf, 0, 1, &vertexBuffer.buf, offsets );
}

void Rhi::CommandList::BindIndexBuffer( const Buffer & indexBuffer, VkIndexType indexType ) {
    vkCmdBindIndexBuffer( mBuf, indexBuffer.buf, 0, indexType );
}

void Rhi::CommandList::Copy( const Buffer & stagingBuf, const Buffer & dstBuf, VkBufferCopy * copy ) {
    vkCmdCopyBuffer( mBuf, stagingBuf.buf, dstBuf.buf, 1, copy );
}

void Rhi::CommandList::Copy( const Buffer & stagingBuf, const Texture & tex, VkBufferImageCopy * copy ) {
    vkCmdCopyBufferToImage( mBuf, stagingBuf.buf, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, copy );
}

void Rhi::CommandList::BufferBarrier( const Buffer & buf ) {
    VkBufferMemoryBarrier2 barrier = {
        .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT,
        .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstAccessMask       = 0,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer              = buf.buf,
        .offset              = 0,
        .size                = buf.size
    };
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    if ( buf.usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ) {
        dstStageMask          |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        barrier.dstAccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    } else if ( buf.usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT ) {
        dstStageMask          |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        barrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
    }

    barrier.dstStageMask = dstStageMask;
    const VkDependencyInfo depInfo = {
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers    = &barrier
    };
    vkCmdPipelineBarrier2( mBuf, &depInfo );
}

void Rhi::CommandList::ImageBarrier( VkImage image, StageAccess src, StageAccess dst, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange range ) {
    const VkImageMemoryBarrier2 barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask        = src.stage,
        .srcAccessMask       = src.access,
        .dstStageMask        = dst.stage,
        .dstAccessMask       = dst.access,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = range,
    };
    const VkDependencyInfo depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier
    };
    vkCmdPipelineBarrier2( mBuf, &depInfo );
}

void Rhi::CommandList::PushConstants( const void * data, uint size ) {
    assert( mBoundRP );
    vkCmdPushConstants( mBuf, mBoundRP->layout, mBoundRP->shaderStage, 0, size, data );
}
