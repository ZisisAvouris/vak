#include <Renderer/Device.hpp>

void Rhi::Device::Init( void ) {
    CreateSurface();
    PickPhysicalDevice( VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU );
    QuerySurfaceCapabilities();
    CreateLogicalDevice();

    VmaAllocatorCreateInfo aci = {
        .flags            = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
        .physicalDevice   = mPhysicalDevice,
        .device           = mLogicalDevice,
        .instance         = RenderContext::Instance()->GetVulkanInstance(),
        .vulkanApiVersion = VK_API_VERSION_1_4
    };
    assert( vmaCreateAllocator( &aci, &mVma ) == VK_SUCCESS );
}

void Rhi::Device::Destroy( void ) {
    vkDestroyDevice( mLogicalDevice, nullptr );
    vkDestroySurfaceKHR( RenderContext::Instance()->GetVulkanInstance(), mSurface, nullptr );
}

void Rhi::Device::CreateSurface( void ) {
    VkWin32SurfaceCreateInfoKHR ci = {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = Core::WindowManager::Instance()->GetWindowInstance(),
        .hwnd      = Core::WindowManager::Instance()->GetWindowHandle()
    };
    assert( vkCreateWin32SurfaceKHR( RenderContext::Instance()->GetVulkanInstance(), &ci, nullptr, &mSurface ) == VK_SUCCESS );
}

void Rhi::Device::QuerySurfaceCapabilities( void ) {
    const VkFormat depthFormats[] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM };
    for ( const VkFormat &depthFormat : depthFormats ) {
        VkFormatProperties fmtProps;
        vkGetPhysicalDeviceFormatProperties( mPhysicalDevice, depthFormat, &fmtProps );

        if ( fmtProps.optimalTilingFeatures )
            mDeviceDepthFormats.push_back( depthFormat );
    }
    mDeviceDepthFormats.shrink_to_fit();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( mPhysicalDevice, mSurface, &mSurfaceCapabilities );

    uint formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( mPhysicalDevice, mSurface, &formatCount, nullptr );
    assert( formatCount > 0 );
    mDeviceSurfaceFormats.resize( formatCount );
    vkGetPhysicalDeviceSurfaceFormatsKHR( mPhysicalDevice, mSurface, &formatCount, mDeviceSurfaceFormats.data() );

    uint presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR( mPhysicalDevice, mSurface, &presentModeCount, nullptr );
    assert( presentModeCount > 0 );
    mDevicePresentModes.resize( presentModeCount );
    vkGetPhysicalDeviceSurfacePresentModesKHR( mPhysicalDevice, mSurface, &presentModeCount, mDevicePresentModes.data() );
}

void Rhi::Device::PickPhysicalDevice( VkPhysicalDeviceType type ) {
    uint deviceCount = 0;
    vkEnumeratePhysicalDevices( RenderContext::Instance()->GetVulkanInstance(), &deviceCount, nullptr );
    vector<VkPhysicalDevice> devices( deviceCount );
    vkEnumeratePhysicalDevices( RenderContext::Instance()->GetVulkanInstance(), &deviceCount, devices.data() );

    for ( uint i = 0; i < deviceCount; ++i ) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties( devices[i], &deviceProperties );

        printf("Found Device [%u] | Name: %s\n", i, deviceProperties.deviceName);
        if ( deviceProperties.deviceType == type ) {
            mPhysicalDevice = devices[i];
            break;
        }
    }
}

void Rhi::Device::CreateLogicalDevice( void ) {
    mQueueFamilyIndex.graphics = FindQueueFamilyIndex( VK_QUEUE_GRAPHICS_BIT );
    printf("Found Graphics Queue with index %u\n", mQueueFamilyIndex.graphics);
    assert( mQueueFamilyIndex.Valid() );

    VkBool32 supportsPresentation = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR( mPhysicalDevice, mQueueFamilyIndex.graphics, mSurface, &supportsPresentation );
    assert( supportsPresentation == VK_TRUE );

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queueCI = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = mQueueFamilyIndex.graphics,
        .queueCount       = 1,
        .pQueuePriorities = &priority
    };

    vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME };

    VkPhysicalDeviceFeatures vkFeatures10 = {
        .samplerAnisotropy = VK_TRUE,
    };

    VkPhysicalDeviceVulkan12Features vkFeatures12 = { // Vulkan 1.2 features we need
        .sType                                        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing                           = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingUpdateUnusedWhilePending    = VK_TRUE,
        .descriptorBindingPartiallyBound              = VK_TRUE,
        .runtimeDescriptorArray                       = VK_TRUE,
        .timelineSemaphore                            = VK_TRUE
    };

    VkPhysicalDeviceVulkan13Features vkFeatures13 = { // Vulkan 1.3 features we need
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext            = &vkFeatures12,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE
    };

    VkDeviceCreateInfo ci = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &vkFeatures13,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &queueCI,
        .enabledExtensionCount   = static_cast<uint>( deviceExtensions.size() ),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures        = &vkFeatures10
    };
    assert( vkCreateDevice( mPhysicalDevice, &ci, nullptr, &mLogicalDevice ) == VK_SUCCESS );
    volkLoadDevice( mLogicalDevice );
    vkGetDeviceQueue( mLogicalDevice, mQueueFamilyIndex.graphics, 0, &mQueues.graphics );
}

uint Rhi::Device::FindQueueFamilyIndex( VkQueueFlags flags ) {
    uint queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( mPhysicalDevice, &queueFamilyCount, nullptr );
    vector<VkQueueFamilyProperties> props( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( mPhysicalDevice, &queueFamilyCount, props.data() );

    for ( uint i = 0; i < queueFamilyCount; ++i ) {
        const bool isSuitable  = ( props[i].queueFlags & flags ) == flags;
        if ( isSuitable && props[i].queueCount )
            return i;
    }

    return UINT32_MAX;
}

Rhi::Buffer Rhi::CreateBuffer( VkBufferUsageFlags usage, VkMemoryPropertyFlags storage, size_t size, const void * data ) {
    Buffer result = {
        .size    = size,
        .usage   = usage,
        .storage = storage
    };

    VkBufferUsageFlags usageFlags = usage;
    if ( result.storage & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
        usageFlags |= ( VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT );
    
    VkBufferCreateInfo ci = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = result.size,
        .usage       = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    
    VmaAllocationCreateInfo allocCI = {};
    if ( result.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
        allocCI.flags          = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        allocCI.requiredFlags  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocCI.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

        assert( vkCreateBuffer( Device::Instance()->GetDevice(), &ci, nullptr, &result.buf ) == VK_SUCCESS );
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements( Device::Instance()->GetDevice(), result.buf, &requirements );
        vkDestroyBuffer( Device::Instance()->GetDevice(), result.buf, nullptr );

        if ( requirements.memoryTypeBits & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT )
            allocCI.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    allocCI.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer( Device::Instance()->GetVMA(), &ci, &allocCI, &result.buf, &result.alloc, nullptr );

    if ( result.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
        vmaMapMemory( Device::Instance()->GetVMA(), result.alloc, &result.ptr );

    if ( data )
        StagingDevice::Instance()->Upload( result, data, size );
    return result;
}

void Rhi::DestroyBuffer( Buffer * buf ) {
    if ( buf->ptr )
        vmaUnmapMemory( Device::Instance()->GetVMA(), buf->alloc );
    vmaDestroyBuffer( Device::Instance()->GetVMA(), buf->buf, buf->alloc );
}

Rhi::Texture Rhi::CreateTexture( const TextureSpecification & spec ) {
    VkImageUsageFlags usageFlags = ( spec.storage == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
    usageFlags |= spec.usage;

    Texture result = {
        .extent     = spec.extent,
        .type       = spec.type,
        .format     = spec.format,
        .usage      = spec.usage,
        .isDepth    = Texture::isDepthFormat( spec.format ),
        .isStencil  = Texture::isStencilFormat( spec.format )
    };
    VkImageCreateInfo ci = {
        .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType             = spec.type,
        .format                = spec.format,
        .extent                = spec.extent,
        .mipLevels             = 1,
        .arrayLayers           = 1,
        .samples               = VK_SAMPLE_COUNT_1_BIT,
        .tiling                = VK_IMAGE_TILING_OPTIMAL,
        .usage                 = usageFlags,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo ai = { .usage = spec.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_AUTO };
    assert( vmaCreateImage( Device::Instance()->GetVMA(), &ci, &ai, &result.image, &result.alloc, nullptr ) == VK_SUCCESS );

    if ( spec.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
        vmaMapMemory( Device::Instance()->GetVMA(), result.alloc, &result.ptr );
    }

    VkImageAspectFlags aspect = 0;
    if ( result.isDepth || result.isStencil ) {
        if ( result.isDepth )   aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if ( result.isStencil ) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    result.view = CreateImageView( result.image, spec.format, aspect );
    if ( spec.data ) {
        StagingDevice::Instance()->Upload( result, spec.data );
    }
    return result;
}

VkImageView Rhi::CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspect ) {
    VkImageViewCreateInfo ci = {
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = image,
        .viewType   = VK_IMAGE_VIEW_TYPE_2D,
        .format     = format,
        .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = {
            .aspectMask     = aspect,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    VkImageView imageView = VK_NULL_HANDLE;
    assert( vkCreateImageView( Device::Instance()->GetDevice(), &ci, nullptr, &imageView ) == VK_SUCCESS );
    return imageView;
}
