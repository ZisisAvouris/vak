#pragma once
#include <Util/Singleton.hpp>
#include <Util/Defines.hpp>
#include <Util/Containers.hpp>
#include <Renderer/RenderBase.hpp>

#include <Resource/Resource.hpp>

namespace Rhi {
    struct Buffer;
    struct RenderPipeline;

    struct StageAccess {
        VkPipelineStageFlags2 stage;
        VkAccessFlags2        access;
    };

    class CommandPool;
    class CommandList final {
    public:
        void BeginRendering( uint2 res, VkImage image, VkImageView view );
        void EndRendering( void );

        void Draw( uint vertexCount, uint instanceCount = 1, uint firstVertex = 0, uint firstInstance = 0 );
        void DrawIndexed( uint indexCount, uint instanceCount = 1, uint firstIndex = 0, uint vertexOffset = 0, uint firstInstance = 0 );

        void BindRenderPipeline( const RenderPipeline & );

        void BindVertexBuffer( const Buffer & );
        void BindIndexBuffer( const Buffer &, VkIndexType indexType = VK_INDEX_TYPE_UINT32 );

        void Copy( const Buffer &, const Buffer &, VkBufferCopy * );
        void Copy( const Buffer &, const Texture &, VkBufferImageCopy * );

        void BufferBarrier( const Buffer & );
        void ImageBarrier( VkImage, StageAccess, StageAccess, VkImageLayout, VkImageLayout, VkImageSubresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } );

        void PushConstants( const void *, uint );

    private:
        friend class CommandPool;
        VkCommandBuffer mBuf = VK_NULL_HANDLE;
        const RenderPipeline * mBoundRP = nullptr;
    };

    class CommandPool final : public Core::Singleton<CommandPool> {
    public:
        void Init( void );
        void Destroy( void );

        CommandList * AcquireCommandList( void );
        void Submit( CommandList *, VkImage );
        void Submit( CommandList *, VkFence );

    private:        
        VkCommandPool mCommandPool;
        CommandList   mCommandList;

    };
}
