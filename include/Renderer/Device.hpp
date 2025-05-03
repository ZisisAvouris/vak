#pragma once
#include <Util/Singleton.hpp>
#include <Renderer/RenderBase.hpp>
#include <Renderer/RenderContext.hpp>
#include <Core/WindowManager.hpp>

#include <vector>

namespace Rhi {

    Buffer CreateBuffer( VkBufferUsageFlags, VkMemoryPropertyFlags, size_t, const void * = nullptr );
    void   DestroyBuffer( Buffer * );

    Texture CreateTexture( const TextureSpecification & );

    VkImageView CreateImageView( VkImage, VkFormat, VkImageAspectFlags );

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

        void Upload( const Buffer &, const void *, size_t );
        void Upload( const Texture &, const void * );

    private:
        Buffer mStagingBuffer;
        uint   mStagingBufferSize     = 0;
        uint   mStagingBufferCapacity = 0;
        uint   mCurrentOffset         = 0;

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

        void CreateSurface( void );

        void PickPhysicalDevice( VkPhysicalDeviceType );
        void CreateLogicalDevice( void );

        uint FindQueueFamilyIndex( VkQueueFlags );
    };

}
