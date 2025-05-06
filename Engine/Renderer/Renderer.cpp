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

    Swapchain::Instance()->Init();
    Swapchain::Instance()->Resize( mRenderResolution );
    ComputeProjectionMatrix();
    Descriptors::Instance()->Init();

    mIsReady = true;

    CommandPool::Instance()->Init();
    PipelineFactory::Instance()->Init();

    const uint pixel = 0xFFFF00FF;
    Device::Instance()->CreateTexture({
        .type      = VK_IMAGE_TYPE_2D,
        .format    = VK_FORMAT_R8G8B8A8_UNORM,
        .extent    = { 1, 1, 1 },
        .usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .data      = &pixel,
        .debugName = "Dummy"
    });
    assert( Device::Instance()->GetTexturePool()->GetEntryCount() == 1 );

    assert( mModel.LoadModelFromFile( "assets/models/sponza/sponza.obj", aiProcess_FlipUVs ) );

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

    depthBuffer = Device::Instance()->CreateTexture({
        .type      = VK_IMAGE_TYPE_2D,
        .format    = VK_FORMAT_D32_SFLOAT,
        .extent    = { mRenderResolution.x, mRenderResolution.y, 1 },
        .usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .debugName = "Depth"
    });

    SamplerMetadata smd = { "Default" };
    sampler = Device::Instance()->CreateSampler({
        .wrapU     = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .wrapV     = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .debugName = "Default"
    });
}

void Rhi::Renderer::Destroy( void ) {
    vkDeviceWaitIdle( Device::Instance()->GetDevice() );

    vkDestroyPipelineLayout( Device::Instance()->GetDevice(), opaquePipeline.layout, nullptr );
    vkDestroyPipeline( Device::Instance()->GetDevice(), opaquePipeline.pipeline, nullptr );

    vkDestroyShaderModule( Device::Instance()->GetDevice(), smVert, nullptr );
    vkDestroyShaderModule( Device::Instance()->GetDevice(), smFrag, nullptr );
    
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

    ComputeProjectionMatrix();
    Device::Instance()->Delete( depthBuffer );
    depthBuffer = Device::Instance()->CreateTexture({
        .type      = VK_IMAGE_TYPE_2D,
        .format    = VK_FORMAT_D32_SFLOAT,
        .extent    = { mRenderResolution.x, mRenderResolution.y, 1 },
        .usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .debugName = "Depth"
    });
    Swapchain::Instance()->Resize( mRenderResolution );
}

void Rhi::Renderer::Render( glm::mat4 view, float deltaTime ) {
    auto [image, imageView] = Swapchain::Instance()->AcquireImage();
    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();

    Descriptors::Instance()->UpdateDescriptorSets();
    cmdlist->BeginRendering( mRenderResolution, image, imageView, depthBuffer );
        cmdlist->BindRenderPipeline( opaquePipeline );
        cmdlist->BeginDebugLabel( "Sponza", { 1.0f, 0.0f, 1.0f, 1.0f } );
        for ( uint i = 0; i < mModel.GetMeshCount(); ++i ) {

            cmdlist->BindVertexBuffer( mModel.GetVertexBuffer( i ) );
            cmdlist->BindIndexBuffer( mModel.GetIndexBuffer( i ) );

            struct PushConstantsBuf {
                glm::mat4 mvp;
                uint      textureId;
            } pc = {
                .mvp       = mProjection * view,
                .textureId = mModel.GetTextureId( i ) 
            };
            cmdlist->PushConstants( &pc, sizeof( PushConstantBuf ) );
            cmdlist->DrawIndexed( mModel.GetIndexCount( i ) );
        }
        cmdlist->EndDebugLabel();
    cmdlist->EndRendering();
    
    CommandPool::Instance()->Submit( cmdlist, image );
    Swapchain::Instance()->Present();
}

void Rhi::Renderer::ComputeProjectionMatrix( void ) {
    // This computes a projection matrix with an infinite far plane for a reverse z buffer
    const float f = 1.0f / tanf( glm::radians( mFov ) / 2.0f );
    mProjection[0][0] = f / ( static_cast<float>( mRenderResolution.x ) / mRenderResolution.y );
    mProjection[1][1] = -f;
    mProjection[2][2] = 0.0f;
    mProjection[2][3] = -1.0f;
    mProjection[3][2] = mNear;

    // alternatively use this, but the far plane may be culling geometry and require a huge zFar
    // mProjection = glm::perspectiveRH_ZO( glm::radians(45.0f), mRenderResolution.x / static_cast<float>(mRenderResolution.y), mFar, mNear );
    // mProjection[1][1] *= -1.0f;
}

void Rhi::Renderer::DebugPrintStructSizes( void ) {
    printf( "[DEBUG] TextureHot  size: %llu\n", sizeof( TextureHot ) );
    printf( "[DEBUG] TextureCold size: %llu\n", sizeof( TextureCold ) );
    printf( "[DEBUG] BufferHot   size: %llu\n", sizeof( BufferHot ) );
    printf( "[DEBUG] BufferCold  size: %llu\n", sizeof( BufferCold ) );
}
