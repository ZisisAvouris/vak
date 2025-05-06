#pragma once
#include <Util/Defines.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#include <volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

#include <string>

namespace Rhi {
    // @todo: Revisit this split of hot and cold data for every resource

    struct BufferSpecification final {
        VkBufferUsageFlags    usage   = 0;
        VkMemoryPropertyFlags storage = 0;
        size_t                size    = 0;
        void                 *ptr     = 0;
        std::string debugName = "You should name this buffer!";
    };
    struct BufferHot final {
        VkBuffer              buf     = VK_NULL_HANDLE;
        VkDeviceSize          size    = 0;
        VkBufferUsageFlags    usage   = 0;
        VkMemoryPropertyFlags storage = 0;
    };
    struct BufferCold final {
        std::string    debugName = "Buffer: ";
        VmaAllocation  alloc     = VK_NULL_HANDLE;
        void          *ptr       = nullptr;
    };

    struct TextureSpecification final {
        VkImageType           type        = VK_IMAGE_TYPE_MAX_ENUM;
        VkFormat              format      = VK_FORMAT_UNDEFINED;
        VkExtent3D            extent      = { 0, 0, 0 };
        VkImageUsageFlags     usage       = 0;
        VkMemoryPropertyFlags storage     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const void *          data        = nullptr; 
        bool                  isSwapchain = false;
        std::string           debugName   = "You should name this texture!";
    };
    struct TextureHot final {
        VkImage           image  = VK_NULL_HANDLE;
        VkImageView       view   = VK_NULL_HANDLE;
        VkExtent3D        extent = { 0, 0, 0 };
        VkImageType       type   = VK_IMAGE_TYPE_MAX_ENUM;
        VkFormat          format = VK_FORMAT_UNDEFINED;
        VkImageLayout     layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageUsageFlags usage  = 0;
    };
    struct TextureCold final {
        std::string    debugName   = "Texture: ";
        VmaAllocation  alloc       = VK_NULL_HANDLE;
        void          *ptr         = nullptr;
        bool           isSwapchain = false;
        bool           isDepth     = false;
        bool           isStencil   = false;
    };
    static inline bool isDepthFormat( VkFormat format ) {
        return ( format == VK_FORMAT_D16_UNORM ) || ( format == VK_FORMAT_X8_D24_UNORM_PACK32 ) || ( format == VK_FORMAT_D32_SFLOAT )
            || ( format == VK_FORMAT_D16_UNORM_S8_UINT ) || ( format == VK_FORMAT_D24_UNORM_S8_UINT ) || ( format == VK_FORMAT_D32_SFLOAT_S8_UINT );
    };
    static inline bool isStencilFormat( VkFormat format ) {
        return ( format == VK_FORMAT_S8_UINT ) || ( format == VK_FORMAT_D16_UNORM_S8_UINT ) || ( format == VK_FORMAT_D24_UNORM_S8_UINT ) || ( format == VK_FORMAT_D32_SFLOAT_S8_UINT );
    }

    struct SamplerSpecification final {
        VkFilter             minFilter = VK_FILTER_LINEAR;
        VkFilter             magFilter = VK_FILTER_LINEAR;
        VkSamplerAddressMode wrapU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        VkSamplerAddressMode wrapV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        std::string          debugName = "You should name this sampler!";
    };
    struct Sampler final {
        VkSampler sampler;
    };
    struct SamplerMetadata final {
        std::string          debugName = "Sampler: ";
        SamplerSpecification spec      = {};
    };

    static constexpr uint sMaxVertexAttributes = 8;
    static constexpr uint sMaxVertexBindings   = 8;
    struct VertexSpecification final {
        struct VertexAttribute final {
            uint location   = 0;
            uint binding    = 0;
            VkFormat format = VK_FORMAT_UNDEFINED;
            uint offset     = 0;
        } attributes[sMaxVertexAttributes];
        struct VertexBinding final {
            uint stride = 0;
        } bindings[sMaxVertexBindings];

        uint GetAttributeCount( void ) const {
            uint count = 0;
            while ( count < sMaxVertexAttributes && attributes[count].format != VK_FORMAT_UNDEFINED )
                ++count;
            return count;
        }

        uint GetBindingCount( void ) const {
            uint count = 0;
            while ( count < sMaxVertexBindings && bindings[count].stride != 0 )
                ++count;
            return count;
        }
    };
    
    struct RenderPipelineSpecification final {
        VkPrimitiveTopology topology       = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VertexSpecification vertexSpec     = {};
        VkShaderModule      vertexShader   = VK_NULL_HANDLE;
        VkShaderModule      fragmentShader = VK_NULL_HANDLE;
        VkCullModeFlags     cullMode       = VK_CULL_MODE_NONE;
        VkFrontFace         winding        = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkPolygonMode       polygonMode    = VK_POLYGON_MODE_FILL;
    };
    struct RenderPipeline final {
        RenderPipelineSpecification spec;
        uint bindingCount              = 0;
        uint attributeCount            = 0;
        VkVertexInputBindingDescription   bindings[sMaxVertexBindings];
        VkVertexInputAttributeDescription attributes[sMaxVertexBindings];
        VkShaderStageFlags shaderStage = 0;
        VkPipelineLayout   layout      = VK_NULL_HANDLE;
        VkPipeline         pipeline    = VK_NULL_HANDLE;
    };

}
