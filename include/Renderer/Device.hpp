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
        uint               mStagingBufferSize     = 0;
        uint               mStagingBufferCapacity = 0;
        uint               mCurrentOffset         = 0;

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

        Util::Pool<Util::_Texture, TextureHot, TextureCold> * GetTexturePool( void ) { return &mTexturePool; }
        Util::Pool<Util::_Buffer, BufferHot, BufferCold> * GetBufferPool( void ) { return &mBufferPool; }

        Util::TextureHandle CreateTexture( const TextureSpecification & );
        Util::BufferHandle  CreateBuffer( const BufferSpecification & );

        void Delete( Util::TextureHandle );
        void Delete( Util::BufferHandle );

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

        Util::Pool<Util::_Texture, TextureHot, TextureCold> mTexturePool{ 128, "Texture" };
        Util::Pool<Util::_Buffer, BufferHot, BufferCold>    mBufferPool{ 128, "Buffer" };

        void CreateSurface( void );

        void PickPhysicalDevice( VkPhysicalDeviceType );
        void CreateLogicalDevice( void );

        uint FindQueueFamilyIndex( VkQueueFlags );
    };

}
