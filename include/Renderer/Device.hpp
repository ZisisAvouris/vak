#pragma once
#include <Util/Singleton.hpp>
#include <Renderer/RenderBase.hpp>
#include <Renderer/RenderContext.hpp>
#include <Core/WindowManager.hpp>
#include <ktx.h>

#include <vector>
#include <algorithm>

namespace Rhi {

    enum QueueType : _byte {
        QueueType_Graphics,
        QueueType_Transfer
    };

    struct DeviceQueues {
        VkQueue graphics = VK_NULL_HANDLE;
        VkQueue transfer = VK_NULL_HANDLE;

        uint graphicsIndex = UINT32_MAX;
        uint transferIndex = UINT32_MAX;

        inline bool Valid( void ) const { return graphicsIndex != UINT32_MAX && transferIndex != UINT32_MAX; }
    };

    class StagingDevice final : public Core::Singleton<StagingDevice> {
    public:
        void Init( void );
        void Destroy( void );

        void Upload( Util::BufferHandle, const void *, size_t );
        void Upload( Util::TextureHandle, const void * );
        void Upload( Util::TextureHandle, ktxTexture2 * );

    private:
        Util::BufferHandle mStagingBuffer;
        size_t             mStagingBufferSize     = 0;
        size_t             mStagingBufferCapacity = 0;
        size_t             mCurrentOffset         = 0;

        VkFence mStagingFence         = VK_NULL_HANDLE;

    };

    using TexturePool = Util::Pool<Util::_Texture, Texture, TextureMetadata>;
    using BufferPool  = Util::Pool<Util::_Buffer, Buffer, BufferMetadata>;
    using SamplerPool = Util::Pool<Util::_Sampler, Sampler, SamplerMetadata>;

    class Device final : public Core::Singleton<Device> {
    public:
        void Init( void );
        void Destroy( void );

        VkPhysicalDevice GetPhysicalDevice( void ) const { return mPhysicalDevice; }
        VkDevice         GetDevice( void ) const { return mLogicalDevice; }

        span<const VkPresentModeKHR> GetPresentModes( void ) const { return mDevicePresentModes; }
        span<const VkSurfaceFormatKHR> GetSurfaceFormats( void ) const { return mDeviceSurfaceFormats; }
        span<const VkFormat> GetDepthFormats( void ) const { return mDeviceDepthFormats; }
        const VkSurfaceCapabilitiesKHR * GetSurfaceCapabilities( void ) const { return &mSurfaceCapabilities; }

        VkSurfaceKHR GetSurface( void ) const { return mSurface; }
        uint         GetQueueIndex( const QueueType type ) const {
            if ( type == QueueType_Graphics ) return mQueues.graphicsIndex;
            if ( type == QueueType_Transfer ) return mQueues.transferIndex;
        }
        VkQueue      GetQueue( const QueueType type ) const {
            if ( type == QueueType_Graphics ) return mQueues.graphics;
            if ( type == QueueType_Transfer ) return mQueues.transfer;
        }

        VmaAllocator GetVMA( void ) const { return mVma; }

        TexturePool * GetTexturePool( void ) { return &mTexturePool; }
        BufferPool  * GetBufferPool( void )  { return &mBufferPool;  }
        SamplerPool * GetSamplerPool( void ) { return &mSamplerPool; }

        Util::TextureHandle CreateTexture( const TextureSpecification & );
        Util::TextureHandle CreateTexture( ktxTexture2 *, const std::string & );
        Util::BufferHandle  CreateBuffer( const BufferSpecification & );
        Util::SamplerHandle CreateSampler( const SamplerSpecification & );

        void Delete( Util::TextureHandle );
        void Delete( Util::BufferHandle );
        void Delete( Util::SamplerHandle );

        ulong DeviceAddress( Util::BufferHandle );

        VkImageView CreateImageView( VkImage, VkFormat, uint, VkImageAspectFlags );

        void QuerySurfaceCapabilities( void );

        void RegisterDebugObjectName( VkObjectType, ulong, const std::string & );

    private:
        VkSurfaceKHR     mSurface        = VK_NULL_HANDLE;
        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
        VkDevice         mLogicalDevice  = VK_NULL_HANDLE;
        DeviceQueues     mQueues         = {};

        VmaAllocator     mVma;

        vector<VkPresentModeKHR>   mDevicePresentModes;
        vector<VkSurfaceFormatKHR> mDeviceSurfaceFormats;
        vector<VkFormat>           mDeviceDepthFormats;
        VkSurfaceCapabilitiesKHR   mSurfaceCapabilities;

        TexturePool        mTexturePool { 128, "Texture" };
        SamplerPool        mSamplerPool {  8, "Sampler" };
        BufferPool         mBufferPool  { 64, "Buffer"  };

        // The dummy textures serves as a placeholder for the bindless array of textures that are not sampled (e.g. swapchain, depth etc),
        // in order to avoid a sparse array and problems with indices
        Util::TextureHandle mDummyTexture;

        void CreateSurface( void );

        void PickPhysicalDevice( VkPhysicalDeviceType );
        void CreateLogicalDevice( void );

        uint FindQueueFamilyIndex( span<const VkQueueFamilyProperties>, VkQueueFlags, VkQueueFlags );
    };

}
