#include <Renderer/Pipeline.hpp>
#include <Renderer/Device.hpp>
#include <Renderer/Swapchain.hpp>
#include <Renderer/Descriptors.hpp>

using namespace std;

void Rhi::PipelineFactory::Init( void ) {
    VkPipelineCacheCreateInfo pcci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
    VK_VERIFY( vkCreatePipelineCache( Device::Instance()->GetDevice(), &pcci, nullptr, &mPipelineCache ) );
}

void Rhi::PipelineFactory::Destroy( void ) {
    for ( uint i = 0; i < mRenderPipelinePool.GetObjectCount(); ++i ) {
        Util::RenderPipelineHandle handle = mRenderPipelinePool.GetHandle( i );
        if ( handle.Valid() ) Delete( handle );
    }
    vkDestroyPipelineCache( Device::Instance()->GetDevice(), mPipelineCache, nullptr );
}

Util::RenderPipelineHandle Rhi::PipelineFactory::CreateRenderPipeline( const RenderPipelineSpecification & spec ) {
    RenderPipelineMetadata metadata = {
        .spec           = spec,
        .attributeCount = spec.vertexSpec.GetAttributeCount()
    };
    metadata.debugName += spec.debugName;

    RenderPipeline rp = {};

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .flags                  = 0,
        .topology               = spec.topology,
        .primitiveRestartEnable = VK_FALSE
    };
    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .flags            = 0,
        .depthClampEnable = VK_FALSE,
        .polygonMode      = spec.polygonMode,
        .cullMode         = spec.cullMode,
        .frontFace        = spec.winding,
        .lineWidth        = 1.f,
    };
    VkPipelineColorBlendAttachmentState blendAttachmentState = {
        .blendEnable         = spec.blend.enable,
        .srcColorBlendFactor = spec.blend.srcRgbBF,
        .dstColorBlendFactor = spec.blend.dstRgbBF,
        .colorBlendOp        = spec.blend.rgbBlendOp,
        .srcAlphaBlendFactor = spec.blend.srcAlphaBF,
        .dstAlphaBlendFactor = spec.blend.dstAlphaBF,
        .alphaBlendOp        = spec.blend.alphaBlendOp,
        .colorWriteMask      = 0xF
    };
    VkPipelineColorBlendAttachmentState colorAttachmentState = {
        .blendEnable    = VK_FALSE,
        .colorWriteMask = 0xF
    };
    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments    = &colorAttachmentState
    };
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .flags         = 0,
        .viewportCount = 1,
        .scissorCount  = 1
    };
    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .flags                = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineDepthStencilStateCreateInfo depthState = {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable  = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp   = VK_COMPARE_OP_GREATER,
    };

    VkDynamicState dynamicStateEnables[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .flags             = 0,
        .dynamicStateCount = VAK_ARRSIZE( dynamicStateEnables ),
        .pDynamicStates    = dynamicStateEnables
    };

    VkFormat surfaceFormat = Swapchain::Instance()->GetSurfaceFormat();
    VkPipelineRenderingCreateInfoKHR pci = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &surfaceFormat,
        .depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT,
    };

    bool isBufferBound[sMaxVertexBindings] = { false };
    for ( uint i = 0; i != metadata.attributeCount; ++i ) {
        const VertexSpecification::VertexAttribute & vattr = spec.vertexSpec.attributes[i];
        metadata.attributes[i] = { .location = vattr.location, .binding = vattr.binding, .format = vattr.format, .offset = vattr.offset };
        if ( !isBufferBound[vattr.binding] ) {
            isBufferBound[vattr.binding] = true;
            metadata.bindings[metadata.bindingCount++] = { .binding = vattr.binding, .stride = spec.vertexSpec.bindings[vattr.binding].stride, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = metadata.bindingCount,
        .pVertexBindingDescriptions      = metadata.bindings,
        .vertexAttributeDescriptionCount = metadata.attributeCount,
        .pVertexAttributeDescriptions    = metadata.attributes
    };

    array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
        VkPipelineShaderStageCreateInfo {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = spec.vertexShader,
            .pName  = "main"
        },
        VkPipelineShaderStageCreateInfo {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = spec.fragmentShader,
            .pName  = "main"
        }
    };

    rp.shaderStage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkPushConstantRange pcRange = {
        .stageFlags = rp.shaderStage,
        .offset     = 0,
        .size       = 256
    };
    VkDescriptorSetLayout dsl = Descriptors::Instance()->GetDescriptorSetLayout();
    VkPipelineLayoutCreateInfo plci = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 1,
        .pSetLayouts            = &dsl,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pcRange
    };
    VK_VERIFY( vkCreatePipelineLayout( Device::Instance()->GetDevice(), &plci, nullptr, &rp.layout ) );

    VkGraphicsPipelineCreateInfo gpci = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &pci,
        .stageCount          = static_cast<uint>( shaderStages.size() ),
        .pStages             = shaderStages.data(),
        .pVertexInputState   = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState   = &multisampleState,
        .pDepthStencilState  = &depthState,
        .pColorBlendState    = &colorBlendState,
        .pDynamicState       = &dynamicState,
        .layout              = rp.layout,
    };

    VK_VERIFY( vkCreateGraphicsPipelines( Device::Instance()->GetDevice(), mPipelineCache, 1, &gpci, nullptr, &rp.pipeline ) );
    return mRenderPipelinePool.Create( std::move( rp ), std::move( metadata ) );
}

void Rhi::PipelineFactory::Delete( Util::RenderPipelineHandle handle ) {
    RenderPipeline * rp = mRenderPipelinePool.Get( handle );
    if ( !rp ) assert( false );
    vkDestroyPipelineLayout( Device::Instance()->GetDevice(), rp->layout, nullptr );
    vkDestroyPipeline( Device::Instance()->GetDevice(), rp->pipeline, nullptr );
}
