#pragma once
#include <Util/Singleton.hpp>
#include <Util/Defines.hpp>
#include <Util/Containers.hpp>
#include <Util/Pool.hpp>
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
        void BeginRendering( uint2 res, VkImage image, VkImageView view, Util::TextureHandle dbHandle );
        void EndRendering( void );

        void Draw( uint vertexCount, uint instanceCount = 1, uint firstVertex = 0, uint firstInstance = 0 );
        void DrawIndexed( uint indexCount, uint instanceCount = 1, uint firstIndex = 0, uint vertexOffset = 0, uint firstInstance = 0 );

        void BindRenderPipeline( const RenderPipeline & );

        void BindVertexBuffer( Util::BufferHandle );
        void BindIndexBuffer( Util::BufferHandle, VkIndexType indexType = VK_INDEX_TYPE_UINT32 );

        void Copy( Util::BufferHandle, Util::BufferHandle, VkBufferCopy * );
        void Copy( Util::BufferHandle, VkImage, VkBufferImageCopy * );

        void BufferBarrier( Util::BufferHandle );
        void ImageBarrier( VkImage, StageAccess, StageAccess, VkImageLayout, VkImageLayout, VkImageSubresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } );

        void PushConstants( const void *, uint );

        void BeginDebugLabel( const char *, const float (&)[4] );
        void EndDebugLabel( void );

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
