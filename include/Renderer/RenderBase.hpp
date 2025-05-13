#pragma once
#include <Util/Defines.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#include <volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

#include <string>
#include <utility>
#include <Util/Pool.hpp>

namespace Rhi {
    using std::pair;
    using std::make_pair;

    // @todo: Revisit this split of hot and cold data for every resource

    struct BufferSpecification final {
        VkBufferUsageFlags    usage   = 0;
        VkMemoryPropertyFlags storage = 0;
        size_t                size    = 0;
        void                 *ptr     = 0;
        std::string debugName = "You should name this buffer!";
    };
    struct Buffer final {
        VkBuffer              buf     = VK_NULL_HANDLE;
        VkDeviceSize          size    = 0;
        VkBufferUsageFlags    usage   = 0;
        VkMemoryPropertyFlags storage = 0;
        VkDeviceAddress       address = 0;
    };
    struct BufferMetadata final {
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
    struct Texture final {
        VkImage           image  = VK_NULL_HANDLE;
        VkImageView       view   = VK_NULL_HANDLE;
        VkExtent3D        extent = { 0, 0, 0 };
        VkImageType       type   = VK_IMAGE_TYPE_MAX_ENUM;
        VkFormat          format = VK_FORMAT_UNDEFINED;
        VkImageLayout     layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageUsageFlags usage  = 0;
    };
    struct TextureMetadata final {
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
    static inline VkImageAspectFlags GetAspectFlags( const TextureMetadata * metadata ) {
        VkImageAspectFlags aspect;
        if ( metadata->isDepth )   aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if ( metadata->isStencil ) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        if ( !metadata->isDepth && !metadata->isStencil ) aspect |= VK_IMAGE_ASPECT_COLOR_BIT;
        return aspect;
    }
    static inline pair<VkPipelineStageFlags2, VkAccessFlags2> GetLayoutFlags( VkImageLayout layout ) {
        switch ( layout ) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                return make_pair( VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0 );
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                return make_pair( VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE );
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                return make_pair( VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT );
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                return make_pair( VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT );
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                return make_pair( VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT );
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                return make_pair( VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT );
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                return make_pair( VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                  VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT );
            case VK_IMAGE_LAYOUT_GENERAL:
                return make_pair( VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT );
            default:
                printf("Invalid Image Layout %u\n", layout);
                assert( false );
        }
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

    struct BlendSettings final {
        VkBool32      enable       = VK_FALSE;
        VkBlendOp     rgbBlendOp   = VK_BLEND_OP_ADD;
        VkBlendFactor srcRgbBF     = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dstRgbBF     = VK_BLEND_FACTOR_ZERO;
        VkBlendOp     alphaBlendOp = VK_BLEND_OP_ADD;
        VkBlendFactor srcAlphaBF   = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dstAlphaBF   = VK_BLEND_FACTOR_ZERO;
    };
    struct RenderPipelineSpecification final {
        VkPrimitiveTopology topology       = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VertexSpecification vertexSpec     = {};
        BlendSettings       blend          = {};
        VkShaderModule      vertexShader   = VK_NULL_HANDLE;
        VkShaderModule      fragmentShader = VK_NULL_HANDLE;
        VkCullModeFlags     cullMode       = VK_CULL_MODE_NONE;
        VkFrontFace         winding        = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkPolygonMode       polygonMode    = VK_POLYGON_MODE_FILL;
        std::string         debugName      = "You should name this render pipeline!";
    };
    struct RenderPipeline final {
        VkPipeline         pipeline    = VK_NULL_HANDLE;
        VkPipelineLayout   layout      = VK_NULL_HANDLE;
        VkShaderStageFlags shaderStage = 0;
    };
    struct RenderPipelineMetadata final {
        std::string                       debugName      = "Render Pipeline: ";
        RenderPipelineSpecification       spec           = {};
        VkVertexInputBindingDescription   bindings[sMaxVertexBindings];
        VkVertexInputAttributeDescription attributes[sMaxVertexAttributes];
        uint                              bindingCount   = 0;
        uint                              attributeCount = 0;
    };

    struct ShaderSpecification final {
        std::string filename  = "";
        std::string debugName = "You should name this shader!";
    };
    struct Shader final {
        VkShaderModule sm = VK_NULL_HANDLE;
    };
    struct ShaderMetadata final {
        std::string         debugName = "Shader: ";
        ShaderSpecification spec      = {};
    };

}
