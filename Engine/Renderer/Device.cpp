#include <Renderer/Device.hpp>
#include <Renderer/Descriptors.hpp>

void Rhi::Device::Init( void ) {
    CreateSurface();
    PickPhysicalDevice( VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU );
    QuerySurfaceCapabilities();
    CreateLogicalDevice();

    VmaAllocatorCreateInfo aci = {
        .flags            = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
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
        .timelineSemaphore                            = VK_TRUE,
        .bufferDeviceAddress                          = VK_TRUE
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

void Rhi::Device::RegisterDebugObjectName( VkObjectType type, ulong handle, const std::string & name ) {
    VkDebugUtilsObjectNameInfoEXT ci = {
        .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType   = type,
        .objectHandle = handle,
        .pObjectName  = name.c_str()
    };
    assert( vkSetDebugUtilsObjectNameEXT( mLogicalDevice, &ci ) == VK_SUCCESS );
};

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

    Texture tex = {
        .extent = spec.extent,
        .type   = spec.type,
        .format = spec.format,
        .usage  = spec.usage
    };
    TextureMetadata metadata = {
        .isDepth   = isDepthFormat( spec.format ),
        .isStencil = isStencilFormat( spec.format )
    };
    metadata.debugName += spec.debugName;

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
    assert( vmaCreateImage( mVma, &ci, &ai, &tex.image, &metadata.alloc, nullptr ) == VK_SUCCESS );
    RegisterDebugObjectName( VK_OBJECT_TYPE_IMAGE, (ulong)tex.image, metadata.debugName + " IMAGE" );

    if ( spec.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
        vmaMapMemory( mVma, metadata.alloc, &metadata.ptr );
    }

    VkImageAspectFlags aspect = 0;
    if ( metadata.isDepth || metadata.isStencil ) {
        if ( metadata.isDepth )   aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if ( metadata.isStencil ) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    tex.view = CreateImageView( tex.image, spec.format, aspect );
    RegisterDebugObjectName( VK_OBJECT_TYPE_IMAGE_VIEW, (ulong)tex.view, metadata.debugName + " VIEW" );

    Util::TextureHandle handle = mTexturePool.Create( std::move( tex ), std::move( metadata ) );
    if ( spec.data ) {
        StagingDevice::Instance()->Upload( handle, spec.data );
    }
    Descriptors::Instance()->SetUpdateDescriptors();
    return handle;
}

void Rhi::Device::Delete( Util::TextureHandle handle ) {
    Texture * tex = mTexturePool.Get( handle );
    TextureMetadata * metadata = mTexturePool.GetMetadata( handle );

    if ( !tex || !metadata )
        return;
    vkDestroyImageView( mLogicalDevice, tex->view, nullptr );
    if ( metadata->ptr )          vmaUnmapMemory( mVma, metadata->alloc );
    if ( !metadata->isSwapchain ) vmaDestroyImage( mVma, tex->image, metadata->alloc );
    mTexturePool.Delete( handle );
}

Util::BufferHandle Rhi::Device::CreateBuffer( const BufferSpecification & spec ) {
    Buffer buf = {
        .size    = spec.size,
        .usage   = spec.usage,
        .storage = spec.storage
    };
    BufferMetadata metadata = {};
    metadata.debugName += spec.debugName;

    VkBufferUsageFlags usageFlags = spec.usage;
    if ( buf.storage & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
        usageFlags |= ( VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT );
    
    VkBufferCreateInfo ci = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = buf.size,
        .usage       = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    
    VmaAllocationCreateInfo allocCI = {};
    if ( buf.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
        allocCI.flags          = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        allocCI.requiredFlags  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocCI.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

        assert( vkCreateBuffer( mLogicalDevice, &ci, nullptr, &buf.buf ) == VK_SUCCESS );
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements( Device::Instance()->GetDevice(), buf.buf, &requirements );
        vkDestroyBuffer( Device::Instance()->GetDevice(), buf.buf, nullptr );

        if ( requirements.memoryTypeBits & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT )
            allocCI.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    allocCI.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer( mVma, &ci, &allocCI, &buf.buf, &metadata.alloc, nullptr );

    RegisterDebugObjectName( VK_OBJECT_TYPE_BUFFER, (ulong)buf.buf, metadata.debugName );

    if ( buf.storage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
        vmaMapMemory( mVma, metadata.alloc, &metadata.ptr );
    
    if ( buf.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT ) {
        const VkBufferDeviceAddressInfo addressInfo = {
            .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buf.buf
        };
        buf.address = vkGetBufferDeviceAddress( mLogicalDevice, &addressInfo );
    }

    Util::BufferHandle handle = mBufferPool.Create( std::move( buf ), std::move( metadata ) );
    if ( spec.ptr )
        StagingDevice::Instance()->Upload( handle, spec.ptr, spec.size );
    return handle;
}

void Rhi::Device::Delete( Util::BufferHandle handle ) {
    Buffer * buf = mBufferPool.Get( handle );
    BufferMetadata * metadata = mBufferPool.GetMetadata( handle );
    if ( !buf || !metadata )
        return;
    if ( metadata->ptr ) vmaUnmapMemory( mVma, metadata->alloc );
    vmaDestroyBuffer( mVma, buf->buf, metadata->alloc );
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

    RegisterDebugObjectName( VK_OBJECT_TYPE_SAMPLER, (ulong)sampler.sampler, metadata.debugName );

    Descriptors::Instance()->SetUpdateDescriptors();
    return mSamplerPool.Create( std::move( sampler ), std::move( metadata ) );
}

void Rhi::Device::Delete( Util::SamplerHandle handle ) {
    Sampler * sampler = mSamplerPool.Get( handle );
    if ( !sampler )
        return;
    vkDestroySampler( mLogicalDevice, sampler->sampler, nullptr );
}

ulong Rhi::Device::DeviceAddress( Util::BufferHandle handle ) {
    Buffer * buf = mBufferPool.Get( handle );
    assert( buf && buf->address );
    return buf->address;
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
