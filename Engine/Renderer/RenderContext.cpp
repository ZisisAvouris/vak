#include <Renderer/RenderContext.hpp>
#include <iostream>

VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, VkDebugUtilsMessageTypeFlagsEXT msgType,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void *userData ) {
    if ( (msgType & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0  ) // Dont print info messages for now
        return VK_FALSE;
    if ( ( msgType & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ) != 0 )
        return VK_FALSE;

    std::cerr << "[ERROR] Vulkan " << data->pMessage << "\n";
    return VK_FALSE;
}

void Rhi::RenderContext::Init( void ) {
    assert( volkInitialize() == VK_SUCCESS );

    VkApplicationInfo ai = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "vak",
        .applicationVersion = VK_MAKE_VERSION( 0, 0, 0 ),
        .pEngineName        = "vak",
        .engineVersion      = VK_MAKE_VERSION( 0, 0, 0 ),
        .apiVersion         = VK_API_VERSION_1_4
    };

    uint instanceLayerPropertiesCount = 0;
    vkEnumerateInstanceLayerProperties( &instanceLayerPropertiesCount, nullptr );
    vector<VkLayerProperties> instanceLayerProperties( instanceLayerPropertiesCount );
    vkEnumerateInstanceLayerProperties( &instanceLayerPropertiesCount, instanceLayerProperties.data() );

    const char *validationLayer = "VK_LAYER_KHRONOS_validation";
    mEnableValidation = find_if( instanceLayerProperties.begin(), instanceLayerProperties.end(), [&validationLayer]( const VkLayerProperties & layer ) {
        return strcmp( layer.layerName, validationLayer ) == 0;
    }) != instanceLayerProperties.end();

    uint instanceExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &instanceExtensionCount, nullptr );
    vector<VkExtensionProperties> instanceExtensions( instanceExtensionCount );
    vkEnumerateInstanceExtensionProperties( nullptr, &instanceExtensionCount, instanceExtensions.data() );

    if ( mEnableValidation ) {
        uint count = 0;
        vkEnumerateInstanceExtensionProperties( validationLayer, &count, nullptr );
        if ( count > 0 ) {
            const size_t size = instanceExtensions.size();
            instanceExtensions.resize( size + count );
            vkEnumerateInstanceExtensionProperties( validationLayer, &count, instanceExtensions.data() + size );
        }
    }

    vector<const char *> instanceExtensionNames = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
    const bool hasDebugUtils = HasExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME, instanceExtensions );
    if ( hasDebugUtils )     instanceExtensionNames.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    if ( mEnableValidation ) instanceExtensionNames.push_back( VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME );

    VkInstanceCreateInfo ci = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &ai,
        .enabledLayerCount       = 1,
        .ppEnabledLayerNames     = &validationLayer,
        .enabledExtensionCount   = static_cast<uint>( instanceExtensionNames.size() ),
        .ppEnabledExtensionNames = instanceExtensionNames.data()
    };
    assert( vkCreateInstance( &ci, nullptr, &mInstance ) == VK_SUCCESS );
    volkLoadInstance( mInstance );

    if ( hasDebugUtils ) {
        VkDebugUtilsMessengerCreateInfoEXT dci = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = &VkDebugCallback,
            .pUserData = this,
        };
        assert( vkCreateDebugUtilsMessengerEXT( mInstance, &dci, nullptr, &mDebugMessenger ) == VK_SUCCESS );
    }
}

void Rhi::RenderContext::Destroy( void ) {
    vkDestroyDebugUtilsMessengerEXT( mInstance, mDebugMessenger, nullptr );
    vkDestroyInstance( mInstance, nullptr );
}

bool Rhi::RenderContext::HasExtension( const char * ext, span<const VkExtensionProperties> properties ) {
    for ( const VkExtensionProperties &p : properties ) {
        if ( strcmp( ext, p.extensionName ) == 0 )
            return true;
    }
    return false;
}
