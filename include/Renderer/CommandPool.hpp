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

    class CommandPool;
    class CommandList final {
    public:
        void BeginRendering( Util::TextureHandle, Util::TextureHandle = {} );
        void EndRendering( void );

        void Draw( uint vertexCount, uint instanceCount = 1, uint firstVertex = 0, uint firstInstance = 0 );
        void DrawIndexed( uint indexCount, uint instanceCount = 1, uint firstIndex = 0, uint vertexOffset = 0, uint firstInstance = 0 );
        void DrawIndexedIndirect( Util::BufferHandle, uint );

        void BindRenderPipeline( Util::RenderPipelineHandle );

        void BindVertexBuffer( Util::BufferHandle );
        void BindIndexBuffer( Util::BufferHandle, VkIndexType indexType = VK_INDEX_TYPE_UINT32 );

        void Copy( Util::BufferHandle, Util::BufferHandle, VkBufferCopy * );
        void Copy( Util::BufferHandle, VkImage, VkBufferImageCopy * );

        void BufferBarrier( Util::BufferHandle, VkPipelineStageFlags2, VkPipelineStageFlags2 );
        void ImageBarrier( Texture *, VkImageLayout );

        void PushConstants( const void *, uint );

        void BeginDebugLabel( const char *, const float (&)[4] );
        void EndDebugLabel( void );

        VkCommandBuffer mBuf            = VK_NULL_HANDLE;
        VkFence         mFence          = VK_NULL_HANDLE;
        VkSemaphore     mSemaphore      = VK_NULL_HANDLE;
        const RenderPipeline * mBoundRP = nullptr;
        bool            mReady          = true;
    };

    class CommandPool final : public Core::Singleton<CommandPool> {
    public:
        void Init( void );
        void Destroy( void );

        CommandList * AcquireCommandList( void );
        void Submit( CommandList *, Util::TextureHandle = {} );

        void SetSwapchainAcquireSemaphore( VkSemaphore acquire ) { mSwapchainAcquireSemaphore = acquire; }

        void WaitAll( void );

    private:
        static constexpr uint sMaxCommandLists = 16;

        VkCommandPool mCommandPool;
        CommandList   mCommandLists[sMaxCommandLists];

        uint          mCommandListCount = sMaxCommandLists;

        VkSemaphore mLastSubmitSemaphore       = VK_NULL_HANDLE;
        VkSemaphore mSwapchainAcquireSemaphore = VK_NULL_HANDLE;
        VkSemaphore mTimelineSemaphore         = VK_NULL_HANDLE;

        void Purge( void );

    };
}
