#include <Renderer/Renderer.hpp>
#include <Renderer/RenderContext.hpp>
#include <Renderer/Device.hpp>
#include <Renderer/Swapchain.hpp>
#include <Renderer/CommandPool.hpp>
#include <Renderer/Pipeline.hpp>
#include <Renderer/Descriptors.hpp>
#include <stb_image.h>

void Rhi::Renderer::Init( uint2 renderResolution ) {
    DebugPrintStructSizes();
    mRenderResolution = renderResolution;

    RenderContext::Instance()->Init();
    Device::Instance()->Init();
    StagingDevice::Instance()->Init();

    Swapchain::Instance()->Init();
    Swapchain::Instance()->Resize( mRenderResolution );
    Descriptors::Instance()->Init();

    mIsReady = true;

    CommandPool::Instance()->Init();
    PipelineFactory::Instance()->Init();

    vector<Vertex> vertexData;
    vertexData.push_back( Vertex { .position = { -1.0f,  1.0f, 0.0f }, .uv = { 0.0f, 0.0f } } );
    vertexData.push_back( Vertex { .position = { -1.0f, -1.0f, 0.0f }, .uv = { 0.0f, 1.0f } } );
    vertexData.push_back( Vertex { .position = {  1.0f, -1.0f, 0.0f }, .uv = { 1.0f, 1.0f } } );

    vector<uint> indexData = { 0, 2, 1 };

    vertexBuffer = Device::Instance()->CreateBuffer({
        .usage     = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .size      = vertexData.size() * sizeof( Vertex ),
        .ptr       = vertexData.data(),
        .debugName = "Vertex" 
    });
    indexBuffer = Device::Instance()->CreateBuffer({
        .usage     = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .size      = indexData.size() * sizeof( Vertex ),
        .ptr       = indexData.data(),
        .debugName = "Index" 
    });

    vertexShader = Resource::LoadShader( "assets/shaders/shader.vert.spv" );
    fragmentShader = Resource::LoadShader( "assets/shaders/shader.frag.spv" );

    VkShaderModuleCreateInfo vertCI = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vertexShader.size,
        .pCode    = vertexShader.byteCode,
    };
    vkCreateShaderModule( Device::Instance()->GetDevice(), &vertCI, nullptr, &smVert );
    VkShaderModuleCreateInfo fragCI = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fragmentShader.size,
        .pCode    = fragmentShader.byteCode
    };
    vkCreateShaderModule( Device::Instance()->GetDevice(), &fragCI, nullptr, &smFrag );
    delete[] vertexShader.byteCode;
    delete[] fragmentShader.byteCode;
    
    opaquePipeline = PipelineFactory::Instance()->CreateRenderPipeline({
        .vertexSpec     = {
            .attributes = { { .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof( Vertex, position ) },
                            { .location = 1, .format = VK_FORMAT_R32G32_SFLOAT,    .offset = offsetof( Vertex, uv ) }
            },
            .bindings = { { .stride = sizeof( Vertex ) } }
        },
        .vertexShader   = smVert,
        .fragmentShader = smFrag,
        .cullMode       = VK_CULL_MODE_BACK_BIT
    });

    texImage = LoadTexture( "assets/textures/wood.jpg" );
    texture  = Device::Instance()->CreateTexture({
        .type      = VK_IMAGE_TYPE_2D,
        .format    = VK_FORMAT_R8G8B8A8_UNORM,
        .extent    = { texImage.width, texImage.height, 1 },
        .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
        .data      = texImage.data,
        .debugName = "Wood"
    });
    stbi_image_free( texImage.data );

    VkSamplerCreateInfo samplerCI = {
        .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter        = VK_FILTER_LINEAR,
        .minFilter        = VK_FILTER_LINEAR,
        .mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias       = .0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy    = 16.0f,
        .compareOp        = VK_COMPARE_OP_NEVER,
        .minLod           = .0f,
        .maxLod           = .0f,
        .borderColor      = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
    };
    vkCreateSampler( Device::Instance()->GetDevice(), &samplerCI, nullptr, &sampler );

    Descriptors::Instance()->UpdateDescriptorSets( texture, sampler );
}

void Rhi::Renderer::Destroy( void ) {
    vkDeviceWaitIdle( Device::Instance()->GetDevice() );

    vkDestroyPipelineLayout( Device::Instance()->GetDevice(), opaquePipeline.layout, nullptr );
    vkDestroyPipeline( Device::Instance()->GetDevice(), opaquePipeline.pipeline, nullptr );

    vkDestroyShaderModule( Device::Instance()->GetDevice(), smVert, nullptr );
    vkDestroyShaderModule( Device::Instance()->GetDevice(), smFrag, nullptr );

    vkDestroySampler( Device::Instance()->GetDevice(), sampler, nullptr );
    
    PipelineFactory::Instance()->Destroy();
    CommandPool::Instance()->Destroy();
    Descriptors::Instance()->Destroy();
    Swapchain::Instance()->Destroy();
    StagingDevice::Instance()->Destroy();
    Device::Instance()->Destroy();
    RenderContext::Instance()->Destroy();
}

void Rhi::Renderer::Resize( uint2 newResolution ) {
    if ( mRenderResolution == newResolution )
        return;

    mRenderResolution = newResolution;
    vkDeviceWaitIdle( Device::Instance()->GetDevice() );
    Swapchain::Instance()->Resize( mRenderResolution );
}

void Rhi::Renderer::Render( float deltaTime ) {
    auto [image, imageView] = Swapchain::Instance()->AcquireImage();
    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();

    static float angle = 0.0f;
    angle += 90.0f * deltaTime;
    angle = fmodf( angle, 360.0f );

    glm::mat4 model = glm::rotate( glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f) );
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective( glm::radians(45.0f), mRenderResolution.x / static_cast<float>(mRenderResolution.y), .1f, 100.0f );

    struct PushConstantsBuf {
        glm::mat4 mvp;
    } pc = {
        .mvp = projection * view * model
    };
    
    cmdlist->BeginRendering( mRenderResolution, image, imageView );
        cmdlist->BindRenderPipeline( opaquePipeline );
        cmdlist->BindVertexBuffer( vertexBuffer );
        cmdlist->BindIndexBuffer( indexBuffer );
        cmdlist->PushConstants( &pc, sizeof(PushConstantsBuf) );
        cmdlist->DrawIndexed( 3 );
    cmdlist->EndRendering();
    
    CommandPool::Instance()->Submit( cmdlist, image );
    Swapchain::Instance()->Present();
}

void Rhi::Renderer::DebugPrintStructSizes( void ) {
    printf( "[DEBUG] TextureHot  size: %llu\n", sizeof( TextureHot ) );
    printf( "[DEBUG] TextureCold size: %llu\n", sizeof( TextureCold ) );
    printf( "[DEBUG] BufferHot   size: %llu\n", sizeof( BufferHot ) );
    printf( "[DEBUG] BufferCold  size: %llu\n", sizeof( BufferCold ) );
}
