#pragma once
#include <Util/Singleton.hpp>
#include <Renderer/Device.hpp>

#include <span>
#include <utility>
#include <assert.h>

namespace Rhi {
    using namespace std;

    class Swapchain final : public Core::Singleton<Swapchain> {
    public:
        void Init( void );
        void Destroy( void );

        void Resize( uint2 );

        std::pair<VkImage, VkImageView> AcquireImage( void );
        void Present( void );

        VkSemaphore GetAcquireSemaphore( void ) const { return mAcquireSemaphore; }
        VkSemaphore GetRenderCompleteSemaphore( void ) const { return mRenderCompleteSemaphore; }
        VkFence GetRenderFence( void ) const { return mRenderFence; }

        VkFormat GetSurfaceFormat( void ) const { return mSurfaceFormat.format; }

    private:
        VkSwapchainKHR     mSwapchain;
        VkFormat           mSwapchainFormat;
        VkSurfaceFormatKHR mSurfaceFormat;
        VkPresentModeKHR   mPresentMode;
        VkImageUsageFlags  mSwapchainUsage;

        uint                mNumSwapchainImages;
        vector<VkImage>     mSwapchainImages;
        vector<VkImageView> mSwapchainViews;

        VkSemaphore         mAcquireSemaphore;
        VkSemaphore         mRenderCompleteSemaphore;
        VkFence             mRenderFence;

        uint                mCurrentImage = 0;
        ulong               mCurrentFrame = 0;

        VkSurfaceFormatKHR PickSwapchainFormat( span<const VkSurfaceFormatKHR> );
        VkPresentModeKHR   PickPresentMode( span<const VkPresentModeKHR> );

    };
}
