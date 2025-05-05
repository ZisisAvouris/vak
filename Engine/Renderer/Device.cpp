#include <Renderer/Device.hpp>
#include <Renderer/Descriptors.hpp>

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

    StagingDevice::Instance()->Init();
}

void Rhi::Device::Destroy( void ) {
    for ( uint i = 0; i < mTexturePool.GetObjectCount(); ++i ) {
        Util::TextureHandle handle = mTexturePool.GetHandle( i );
        if ( handle.Valid() ) Delete( handle );
    }
    for ( uint i = 0; i < mBufferPool.GetObjectCount(); ++i ) {
        Util::BufferHandle handle = mBufferPool.GetHandle( i );
        if ( handle.Valid() ) Delete( handle );
    }
    for ( uint i = 0; i < mSamplerPool.GetObjectCount(); ++i ) {
        Util::SamplerHandle handle = mSamplerPool.GetHandle( i );
        if ( handle.Valid() ) Delete( handle );
    }

    vmaDestroyAllocator( mVma );
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

Util::TextureHandle Rhi::Device::CreateTexture( const TextureSpecification & spec ) {
    VkImageUsageFlags usageFlags = ( spec.storage == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
    usageFlags |= spec.usage;

    TextureHot hotResult = {
        .extent = spec.extent,
        .type   = spec.type,
        .format = spec.format,
        .usage  = spec.usage
    };
    TextureCold coldResult = {
        .isDepth   = isDepthFormat( spec.format ),
        .isStencil = isStencilFormat( spec.format )
    };
    coldResult.debugName += spec.debugName;

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
    assert( vmaCreateImage( mVma, &ci, &ai, &hotResult.image, &coldResult.alloc, nullptr ) == VK_SUCCESS );

    if ( spec.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
        vmaMapMemory( mVma, coldResult.alloc, &coldResult.ptr );
    }

    VkImageAspectFlags aspect = 0;
    if ( coldResult.isDepth || coldResult.isStencil ) {
        if ( coldResult.isDepth )   aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if ( coldResult.isStencil ) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    hotResult.view = CreateImageView( hotResult.image, spec.format, aspect );
    Util::TextureHandle handle = mTexturePool.Create( std::move( hotResult ), std::move( coldResult ) );
    if ( spec.data ) {
        StagingDevice::Instance()->Upload( handle, spec.data );
    }
    Descriptors::Instance()->SetUpdateDescriptors();
    return handle;
}

void Rhi::Device::Delete( Util::TextureHandle handle ) {
    TextureHot * hot = mTexturePool.GetHot( handle );
    TextureCold * cold = mTexturePool.GetCold( handle );

    if ( !hot || !cold )
        return;
    vkDestroyImageView( mLogicalDevice, hot->view, nullptr );
    if ( cold->ptr )          vmaUnmapMemory( mVma, cold->alloc );
    if ( !cold->isSwapchain ) vmaDestroyImage( mVma, hot->image, cold->alloc );
    mTexturePool.Delete( handle );
}

Util::BufferHandle Rhi::Device::CreateBuffer( const BufferSpecification & spec ) {
    BufferHot hotResult = {
        .size    = spec.size,
        .usage   = spec.usage,
        .storage = spec.storage
    };
    BufferCold coldResult = {};
    coldResult.debugName += spec.debugName;

    VkBufferUsageFlags usageFlags = spec.usage;
    if ( hotResult.storage & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
        usageFlags |= ( VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT );
    
    VkBufferCreateInfo ci = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = hotResult.size,
        .usage       = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    
    VmaAllocationCreateInfo allocCI = {};
    if ( hotResult.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
        allocCI.flags          = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        allocCI.requiredFlags  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocCI.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

        assert( vkCreateBuffer( mLogicalDevice, &ci, nullptr, &hotResult.buf ) == VK_SUCCESS );
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements( Device::Instance()->GetDevice(), hotResult.buf, &requirements );
        vkDestroyBuffer( Device::Instance()->GetDevice(), hotResult.buf, nullptr );

        if ( requirements.memoryTypeBits & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT )
            allocCI.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    allocCI.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer( mVma, &ci, &allocCI, &hotResult.buf, &coldResult.alloc, nullptr );

    if ( hotResult.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
        vmaMapMemory( mVma, coldResult.alloc, &coldResult.ptr );

    Util::BufferHandle handle = mBufferPool.Create( std::move( hotResult ), std::move( coldResult ) );
    if ( spec.ptr )
        StagingDevice::Instance()->Upload( handle, spec.ptr, spec.size );
    return handle;
}

void Rhi::Device::Delete( Util::BufferHandle handle ) {
    BufferHot * hot = mBufferPool.GetHot( handle );
    BufferCold * cold = mBufferPool.GetCold( handle );
    if ( !hot || !cold )
        return;
    if ( cold->ptr ) vmaUnmapMemory( mVma, cold->alloc );
    vmaDestroyBuffer( mVma, hot->buf, cold->alloc );
    mBufferPool.Delete( handle );
}

Util::SamplerHandle Rhi::Device::CreateSampler( const SamplerSpecification & spec ) {
    VkSamplerCreateInfo ci = {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter               = spec.magFilter,
        .minFilter               = spec.minFilter,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU            = spec.wrapU,
        .addressModeV            = spec.wrapV,
        .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias              = .0f,
        .anisotropyEnable        = VK_TRUE,
        .maxAnisotropy           = 16.0f,
        .compareOp               = VK_COMPARE_OP_NEVER,
        .minLod                  = .0f,
        .maxLod                  = .0f,
        .borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
    };
    Sampler sampler;
    assert( vkCreateSampler( mLogicalDevice, &ci, nullptr, &sampler.sampler ) == VK_SUCCESS );
    SamplerMetadata metadata = { .spec = spec };
    metadata.debugName += spec.debugName;
    Descriptors::Instance()->SetUpdateDescriptors();
    return mSamplerPool.Create( std::move( sampler ), std::move( metadata ) );
}

void Rhi::Device::Delete( Util::SamplerHandle handle ) {
    Sampler * sampler = mSamplerPool.GetHot( handle );
    if ( !sampler )
        return;
    vkDestroySampler( mLogicalDevice, sampler->sampler, nullptr );
}

VkImageView Rhi::Device::CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspect ) {
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
    assert( vkCreateImageView( mLogicalDevice, &ci, nullptr, &imageView ) == VK_SUCCESS );
    return imageView;
}
