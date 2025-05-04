#include <Renderer/Swapchain.hpp>

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
        vkDestroyImageView( Device::Instance()->GetDevice(), mSwapchainViews[i], nullptr );
    }
    vkDestroySemaphore( Device::Instance()->GetDevice(), mAcquireSemaphore, nullptr );
    vkDestroySemaphore( Device::Instance()->GetDevice(), mRenderCompleteSemaphore, nullptr );
    vkDestroyFence( Device::Instance()->GetDevice(), mRenderFence, nullptr );
    vkDestroySwapchainKHR( Device::Instance()->GetDevice(), mSwapchain, nullptr );
    mSwapchain = VK_NULL_HANDLE;
}

void Rhi::Swapchain::Resize( uint2 newResolution ) {
    if ( mSwapchain )
        Destroy();
    Device::Instance()->QuerySurfaceCapabilities();
    
    const uint minImages = Device::Instance()->GetSurfaceCapabilities()->minImageCount + 1;
    const uint maxImages = Device::Instance()->GetSurfaceCapabilities()->maxImageCount;

    mNumSwapchainImages = ( minImages > maxImages ) ? maxImages : minImages;

    const uint graphicsFamilyIndex = Device::Instance()->GetQueueIndex();
    VkSwapchainCreateInfoKHR ci = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = Device::Instance()->GetSurface(),
        .minImageCount         = mNumSwapchainImages,
        .imageFormat           = mSurfaceFormat.format,
        .imageColorSpace       = mSurfaceFormat.colorSpace,
        .imageExtent           = { newResolution.x, newResolution.y },
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

    mSwapchainImages.resize( mNumSwapchainImages ); 
    vkGetSwapchainImagesKHR( Device::Instance()->GetDevice(), mSwapchain, &mNumSwapchainImages, mSwapchainImages.data() );

    mSwapchainViews.resize( mNumSwapchainImages );
    for ( uint i = 0; i < mNumSwapchainImages; ++i ) {
        VkImageViewCreateInfo ivci = {
            .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image      = mSwapchainImages[i],
            .viewType   = VK_IMAGE_VIEW_TYPE_2D,
            .format     = mSwapchainFormat,
            .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            }
        };
        assert( vkCreateImageView( Device::Instance()->GetDevice(), &ivci, nullptr, &mSwapchainViews[i] ) == VK_SUCCESS );
    }
    VkSemaphoreCreateInfo sci = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    assert( vkCreateSemaphore( Device::Instance()->GetDevice(), &sci, nullptr, &mAcquireSemaphore ) == VK_SUCCESS );
    assert( vkCreateSemaphore( Device::Instance()->GetDevice(), &sci, nullptr, &mRenderCompleteSemaphore ) == VK_SUCCESS );

    VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    assert( vkCreateFence( Device::Instance()->GetDevice(), &fci, nullptr, &mRenderFence ) == VK_SUCCESS );
}

std::pair<VkImage, VkImageView> Rhi::Swapchain::AcquireImage( void ) {
    vkWaitForFences( Device::Instance()->GetDevice(), 1, &mRenderFence, VK_TRUE, UINT64_MAX );
    vkResetFences( Device::Instance()->GetDevice(), 1, &mRenderFence );

    assert( vkAcquireNextImageKHR( Device::Instance()->GetDevice(), mSwapchain, UINT64_MAX, mAcquireSemaphore, VK_NULL_HANDLE, &mCurrentImage ) == VK_SUCCESS );
    return std::make_pair( mSwapchainImages[mCurrentImage], mSwapchainViews[mCurrentImage] );
}

void Rhi::Swapchain::Present( void ) {
    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &mRenderCompleteSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &mSwapchain,
        .pImageIndices      = &mCurrentImage
    };
    vkQueuePresentKHR( Device::Instance()->GetQueue(), &presentInfo );
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
