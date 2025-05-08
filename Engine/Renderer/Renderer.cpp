#include <Renderer/Renderer.hpp>
#include <Renderer/RenderContext.hpp>
#include <Renderer/Device.hpp>
#include <Renderer/Swapchain.hpp>
#include <Renderer/CommandPool.hpp>
#include <Renderer/Pipeline.hpp>
#include <Renderer/Descriptors.hpp>
#include <Renderer/GUI.hpp>
#include <Core/Input.hpp>
#include <stb_image.h>

void Rhi::Renderer::Init( uint2 renderResolution, void * windowHandle ) {
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
    GUI::Renderer::Instance()->Init( windowHandle );

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

    assert( mModel.LoadModelFromFile( "assets/models/sponza/sponza.obj", aiProcess_FlipUVs | aiProcess_GenSmoothNormals ) );

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
                            { .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof( Vertex, normal ) },
                            { .location = 2, .format = VK_FORMAT_R32G32_SFLOAT,    .offset = offsetof( Vertex, uv ) }
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

    vector<Entity::PointLight> pointLights = {
        Entity::PointLight {
            .position  = glm::vec3( -300.0f, 200.0f, 0.0f ),
            .color     = glm::vec3( 1.0f, 0.0f, 0.0f ),
            .intensity = 10.0f,
            .linear    = 0.002f,
            .quadratic = 0.00005f
        },
        Entity::PointLight {
            .position  = glm::vec3( 300.0f, 200.0f, 0.0f ),
            .color     = glm::vec3( 0.0f, 0.0f, 1.0f ),
            .intensity = 10.0f,
            .linear    = 0.002f,
            .quadratic = 0.00005f
        }
    };
    lightCount = pointLights.size();

    lightBuffer = Device::Instance()->CreateBuffer({
        .usage     = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .size      = lightCount * sizeof( Entity::PointLight ),
        .ptr       = pointLights.data(),
        .debugName = "Point Lights"
    });
}

void Rhi::Renderer::Destroy( void ) {
    vkDeviceWaitIdle( Device::Instance()->GetDevice() );

    vkDestroyPipelineLayout( Device::Instance()->GetDevice(), opaquePipeline.layout, nullptr );
    vkDestroyPipeline( Device::Instance()->GetDevice(), opaquePipeline.pipeline, nullptr );

    vkDestroyShaderModule( Device::Instance()->GetDevice(), smVert, nullptr );
    vkDestroyShaderModule( Device::Instance()->GetDevice(), smFrag, nullptr );
    
    GUI::Renderer::Instance()->Destroy();
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

void Rhi::Renderer::Render( glm::vec3 cameraPosition, glm::mat4 view, float deltaTime ) {
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
                uint      lightCount;
                ulong     lightBuffer;
                glm::vec3 cameraPosition;
            } pc = {
                .mvp            = mProjection * view,
                .textureId      = mModel.GetTextureId( i ),
                .lightCount     = lightCount,
                .lightBuffer    = Device::Instance()->DeviceAddress( lightBuffer ),
                .cameraPosition = cameraPosition
            };
            static_assert( sizeof( PushConstantsBuf ) <= 128 );

            cmdlist->PushConstants( &pc, sizeof( PushConstantsBuf ) );
            cmdlist->DrawIndexed( mModel.GetIndexCount( i ) );
        }
        cmdlist->EndDebugLabel();

        if ( Input::KeyboardInputs::Instance()->GetKey( Input::Key_G ) )
            GUI::Renderer::Instance()->Render( cmdlist );
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
    printf( "[DEBUG] TextureHot      size: %llu\n", sizeof( Texture ) );
    printf( "[DEBUG] TextureCold     size: %llu\n", sizeof( TextureMetadata ) );
    printf( "[DEBUG] Buffer          size: %llu\n", sizeof( Buffer ) );
    printf( "[DEBUG] BufferMetadata  size: %llu\n", sizeof( BufferMetadata ) );
    printf( "[DEBUG] Sampler         size: %llu\n", sizeof( Sampler ) );
    printf( "[DEBUG] SamplerMetadata size: %llu\n", sizeof( SamplerMetadata ) );
}
