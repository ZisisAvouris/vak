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

        Util::TextureHandle AcquireImage( void );
        void Present( VkSemaphore );

        VkSemaphore GetAcquireSemaphore( void ) const { return mAcquireSemaphores[mCurrentFrame]; }
        VkSemaphore GetRenderCompleteSemaphore( void ) const { return mRenderCompleteSemaphores[mCurrentFrame]; }

        VkFormat GetSurfaceFormat( void ) const { return mSurfaceFormat.format; }
        VkExtent2D GetSwapchainExtent( void ) const { return mSwapchainExtent; }

        ulong GetCurrentFrame( void ) const { return mCurrentFrame; }
        uint  GetImageCount( void ) const { return mNumSwapchainImages; }

        void SetWaitValue( ulong value ) { mTimelineWaitValues[mCurrentImage] = value; }

    private:
        VkSwapchainKHR     mSwapchain;
        VkFormat           mSwapchainFormat;
        VkSurfaceFormatKHR mSurfaceFormat;
        VkPresentModeKHR   mPresentMode;
        VkImageUsageFlags  mSwapchainUsage;
        VkExtent2D         mSwapchainExtent;

        uint                        mNumSwapchainImages;
        vector<Util::TextureHandle> mSwapchainImages;

        VkSemaphore         mAcquireSemaphores[3];
        VkSemaphore         mRenderCompleteSemaphores[3];
        ulong               mTimelineWaitValues[3] = { 0 };

        uint                mCurrentImage = 0;
        ulong               mCurrentFrame = 0;

        VkSurfaceFormatKHR PickSwapchainFormat( span<const VkSurfaceFormatKHR> );
        VkPresentModeKHR   PickPresentMode( span<const VkPresentModeKHR> );

    };
}
