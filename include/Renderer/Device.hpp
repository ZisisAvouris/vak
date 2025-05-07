#pragma once
#include <Util/Singleton.hpp>
#include <Renderer/RenderBase.hpp>
#include <Renderer/RenderContext.hpp>
#include <Core/WindowManager.hpp>

#include <vector>

namespace Rhi {

    struct QueueFamilyIndex {
        uint graphics = UINT32_MAX;

        inline bool Valid( void ) const { return graphics != UINT32_MAX; } 
    };

    struct DeviceQueues {
        VkQueue graphics = VK_NULL_HANDLE;
    };

    class StagingDevice final : public Core::Singleton<StagingDevice> {
    public:
        void Init( void );
        void Destroy( void );

        void Upload( Util::BufferHandle, const void *, size_t );
        void Upload( Util::TextureHandle, const void * );

    private:
        Util::BufferHandle mStagingBuffer;
        size_t             mStagingBufferSize     = 0;
        size_t             mStagingBufferCapacity = 0;
        size_t             mCurrentOffset         = 0;

        VkFence mStagingFence         = VK_NULL_HANDLE;

    };

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
        uint         GetQueueIndex( void ) const { return mQueueFamilyIndex.graphics; } // @todo: revisit
        VkQueue      GetQueue( void ) const { return mQueues.graphics; }

        VmaAllocator GetVMA( void ) const { return mVma; }

        Util::Pool<Util::_Texture, Texture, TextureMetadata> * GetTexturePool( void ) { return &mTexturePool; }
        Util::Pool<Util::_Buffer, Buffer, BufferMetadata> * GetBufferPool( void ) { return &mBufferPool; }
        Util::Pool<Util::_Sampler, Sampler, SamplerMetadata> * GetSamplerPool( void ) { return &mSamplerPool; }

        Util::TextureHandle CreateTexture( const TextureSpecification & );
        Util::BufferHandle  CreateBuffer( const BufferSpecification & );
        Util::SamplerHandle CreateSampler( const SamplerSpecification & );

        void Delete( Util::TextureHandle );
        void Delete( Util::BufferHandle );
        void Delete( Util::SamplerHandle );

        ulong DeviceAddress( Util::BufferHandle );

        VkImageView CreateImageView( VkImage, VkFormat, VkImageAspectFlags );

        void QuerySurfaceCapabilities( void );
    
    private:
        VkSurfaceKHR     mSurface        = VK_NULL_HANDLE;
        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
        VkDevice         mLogicalDevice  = VK_NULL_HANDLE;

        QueueFamilyIndex mQueueFamilyIndex = {};
        DeviceQueues     mQueues           = {};

        VmaAllocator     mVma;

        vector<VkPresentModeKHR>   mDevicePresentModes;
        vector<VkSurfaceFormatKHR> mDeviceSurfaceFormats;
        vector<VkFormat>           mDeviceDepthFormats;
        VkSurfaceCapabilitiesKHR   mSurfaceCapabilities;

        Util::Pool<Util::_Texture, Texture, TextureMetadata>  mTexturePool{ 64, "Texture" };
        Util::Pool<Util::_Sampler, Sampler, SamplerMetadata> mSamplerPool{ 8, "Sampler" };
        Util::Pool<Util::_Buffer, Buffer, BufferMetadata>    mBufferPool{ 1024, "Buffer" };

        // The dummy textures serves as a placeholder for the bindless array of textures that are not sampled (e.g. swapchain, depth etc),
        // in order to avoid a sparse array and problems with indices
        Util::TextureHandle mDummyTexture;

        void CreateSurface( void );

        void PickPhysicalDevice( VkPhysicalDeviceType );
        void CreateLogicalDevice( void );

        void RegisterDebugObjectName( VkObjectType, ulong, const std::string & );

        uint FindQueueFamilyIndex( VkQueueFlags );
    };

}
