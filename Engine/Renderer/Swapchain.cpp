#include <Renderer/Swapchain.hpp>
#include <Renderer/Timeline.hpp>
#include <Renderer/CommandPool.hpp>

void Rhi::Swapchain::Init( void ) {
    mSurfaceFormat   = PickSwapchainFormat( Device::Instance()->GetSurfaceFormats() );
    mSwapchainFormat = mSurfaceFormat.format;
    mPresentMode     = PickPresentMode( Device::Instance()->GetPresentModes() );

    mSwapchainUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VkFormatProperties props = {};
    vkGetPhysicalDeviceFormatProperties( Device::Instance()->GetPhysicalDevice(), mSwapchainFormat, &props );

    const bool isStorageSupported = ( Device::Instance()->GetSurfaceCapabilities()->supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT ) > 0;
    const bool isTilingOptimal    = ( props.optimalTilingFeatures & VK_IMAGE_USAGE_STORAGE_BIT ) > 0;

    if ( isStorageSupported && isTilingOptimal )
        mSwapchainUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
}

void Rhi::Swapchain::Destroy( void ) {
    for ( uint i = 0; i < mNumSwapchainImages; ++i ) {
        vkDestroySemaphore( Device::Instance()->GetDevice(), mAcquireSemaphores[i], nullptr );
        vkDestroySemaphore( Device::Instance()->GetDevice(), mRenderCompleteSemaphores[i], nullptr );
    }
    vkDestroySwapchainKHR( Device::Instance()->GetDevice(), mSwapchain, nullptr );
    mSwapchain = VK_NULL_HANDLE;
}

void Rhi::Swapchain::Resize( uint2 newResolution ) {
    if ( mSwapchain )
        Destroy();
    Device::Instance()->QuerySurfaceCapabilities();
    mSwapchainExtent = { newResolution.x, newResolution.y };

    const uint minImages = Device::Instance()->GetSurfaceCapabilities()->minImageCount + 1;
    const uint maxImages = Device::Instance()->GetSurfaceCapabilities()->maxImageCount;

    mNumSwapchainImages = ( minImages > maxImages ) ? maxImages : minImages;

    const uint graphicsFamilyIndex = Device::Instance()->GetQueueIndex( QueueType_Graphics );
    VkSwapchainCreateInfoKHR ci = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = Device::Instance()->GetSurface(),
        .minImageCount         = mNumSwapchainImages,
        .imageFormat           = mSurfaceFormat.format,
        .imageColorSpace       = mSurfaceFormat.colorSpace,
        .imageExtent           = mSwapchainExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = mSwapchainUsage,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices   = &graphicsFamilyIndex,
        .preTransform          = Device::Instance()->GetSurfaceCapabilities()->currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = mPresentMode,
        .clipped               = VK_TRUE,
        .oldSwapchain          = VK_NULL_HANDLE
    };
    assert( vkCreateSwapchainKHR( Device::Instance()->GetDevice(), &ci, nullptr, &mSwapchain ) == VK_SUCCESS );

    VkImage swapchainImages[4];
    mSwapchainImages.resize( mNumSwapchainImages );
    vkGetSwapchainImagesKHR( Device::Instance()->GetDevice(), mSwapchain, &mNumSwapchainImages, swapchainImages );

    VkImageView swapchainViews[4];
    for ( uint i = 0; i < mNumSwapchainImages; ++i ) {
        VkImageViewCreateInfo ivci = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = swapchainImages[i],
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = mSwapchainFormat,
            .components       = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            }
        };
        assert( vkCreateImageView( Device::Instance()->GetDevice(), &ivci, nullptr, &swapchainViews[i] ) == VK_SUCCESS );

        Texture tex = {
            .image  = swapchainImages[i],
            .view   = swapchainViews[i],
            .extent = { newResolution.x, newResolution.y, 1 },
            .type   = VK_IMAGE_TYPE_2D,
            .format = mSwapchainFormat
        };
        TextureMetadata metadata = {
            .debugName   = "Swapchain " + std::to_string(i),
            .isSwapchain = true,
        };
        mSwapchainImages[i] = Device::Instance()->GetTexturePool()->Create( std::move( tex ), std::move( metadata ) );

        VkSemaphoreCreateInfo sci = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        assert( vkCreateSemaphore( Device::Instance()->GetDevice(), &sci, nullptr, &mAcquireSemaphores[i] ) == VK_SUCCESS );
        Device::Instance()->RegisterDebugObjectName( VK_OBJECT_TYPE_SEMAPHORE, (ulong)mAcquireSemaphores[i], "Swapchain Acquire Semaphore " + std::to_string(i) );
    }
}

Util::TextureHandle Rhi::Swapchain::AcquireImage( void ) {
    VkSemaphore timeline = Timeline::Instance()->GetTimeline();
    VkSemaphoreWaitInfo waitSemaphore = {
        .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR,
        .semaphoreCount = 1,
        .pSemaphores    = &timeline,
        .pValues        = &mTimelineWaitValues[mCurrentImage]
    };
    assert( vkWaitSemaphores( Device::Instance()->GetDevice(), &waitSemaphore, UINT64_MAX ) == VK_SUCCESS );

    VkSemaphore acquire = mAcquireSemaphores[mCurrentImage];
    assert( vkAcquireNextImageKHR( Device::Instance()->GetDevice(), mSwapchain, UINT64_MAX, acquire, VK_NULL_HANDLE, &mCurrentImage ) == VK_SUCCESS );
    CommandPool::Instance()->SetSwapchainAcquireSemaphore( acquire );
    return mSwapchainImages[mCurrentImage];
}

void Rhi::Swapchain::Present( VkSemaphore submitWaitSemaphore ) {
    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &submitWaitSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &mSwapchain,
        .pImageIndices      = &mCurrentImage
    };
    vkQueuePresentKHR( Device::Instance()->GetQueue( QueueType_Graphics ), &presentInfo );
    Timeline::Instance()->IncrementFrame();
}

VkSurfaceFormatKHR Rhi::Swapchain::PickSwapchainFormat( span<const VkSurfaceFormatKHR> formats ) {
    for ( const VkSurfaceFormatKHR & format : formats ) {
        if ( format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
            return format;
    }
    assert( false ); // @todo: revisit
}

VkPresentModeKHR Rhi::Swapchain::PickPresentMode( span<const VkPresentModeKHR> modes ) {
    for ( const VkPresentModeKHR & mode : modes ) {
        if ( mode == VK_PRESENT_MODE_MAILBOX_KHR )
            return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}
