#include <Renderer/Renderer.hpp>
#include <Renderer/RenderContext.hpp>
#include <Renderer/Device.hpp>
#include <Renderer/Swapchain.hpp>
#include <Renderer/CommandPool.hpp>
#include <Renderer/Pipeline.hpp>
#include <Renderer/Descriptors.hpp>
#include <Renderer/GUI.hpp>
#include <Renderer/Shader.hpp>
#include <Renderer/RenderStatistics.hpp>
#include <Renderer/Timeline.hpp>
#include <Core/SceneGraph.hpp>
#include <Core/Input.hpp>
#include <Core/JobSystem.hpp>
#include <stb_image.h>

void Rhi::Renderer::Init( uint2 renderResolution, void * windowHandle ) {
    DebugPrintStructSizes();
    mRenderResolution = renderResolution;

    RenderContext::Instance()->Init();
    Device::Instance()->Init();
    CommandPool::Instance()->Init();
    Timeline::Instance()->Init();

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

    Swapchain::Instance()->Init();
    Swapchain::Instance()->Resize( mRenderResolution );
    Timeline::Instance()->SignalTimeline( Swapchain::Instance()->GetImageCount() - 1 );
    ComputeProjectionMatrix();
    Descriptors::Instance()->Init();

    mIsReady = true;

    PipelineFactory::Instance()->Init();
    ShaderManager::Instance()->Init();
    GUI::Renderer::Instance()->Init( windowHandle );
    Core::JobSystem::Instance()->Init();

    const bool sponzaOK = mSponza.LoadMeshFromFile( "assets/models/modern_sponza/NewSponza_Main_glTF_003.gltf", true, aiProcess_FlipUVs | aiProcess_GenSmoothNormals );
    const bool curtainsOK = mCurtains.LoadMeshFromFile( "assets/models/modern_sponza_curtains/NewSponza_Curtains_glTF.gltf", true, aiProcess_FlipUVs | aiProcess_GenSmoothNormals );

    shMainVert = ShaderManager::Instance()->LoadShader( { "assets/shaders/shader.vert.spv", "Main Vertex" } );
    shMainFrag = ShaderManager::Instance()->LoadShader( { "assets/shaders/shader.frag.spv", "Main Fragment" } );

    pipelineOpaque = PipelineFactory::Instance()->CreateRenderPipeline({
        .vertexSpec     = {
            .attributes = { { .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof( Vertex, position ) },
                            { .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof( Vertex, normal ) },
                            { .location = 2, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof( Vertex, tangent ) },
                            { .location = 3, .format = VK_FORMAT_R32G32_SFLOAT,    .offset = offsetof( Vertex, uv ) }
            },
            .bindings = { { .stride = sizeof( Vertex ) } }
        },
        .blend = {
            .enable     = VK_TRUE,
            .srcRgbBF   = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstRgbBF   = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .srcAlphaBF = VK_BLEND_FACTOR_ONE,
            .dstAlphaBF = VK_BLEND_FACTOR_ZERO
        },
        .vertexShader   = ShaderManager::Instance()->GetShaderPool()->Get( shMainVert )->sm,
        .fragmentShader = ShaderManager::Instance()->GetShaderPool()->Get( shMainFrag )->sm,
        .cullMode       = VK_CULL_MODE_BACK_BIT
    });
    pipelinePlane = PipelineFactory::Instance()->CreateRenderPipeline({
        .vertexSpec     = {
            .attributes = { { .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof( Vertex, position ) },
                            { .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof( Vertex, normal ) },
                            { .location = 2, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof( Vertex, tangent ) },
                            { .location = 3, .format = VK_FORMAT_R32G32_SFLOAT,    .offset = offsetof( Vertex, uv ) }
            },
            .bindings = { { .stride = sizeof( Vertex ) } }
        },
        .vertexShader   = ShaderManager::Instance()->GetShaderPool()->Get( shMainVert )->sm,
        .fragmentShader = ShaderManager::Instance()->GetShaderPool()->Get( shMainFrag )->sm
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
            .position  = glm::vec3( -10.0f, 3.0f, -1.0f ),
            .color     = glm::vec3( 1.0f, 1.0f, 1.0f ),
            .intensity = 3.0f,
            .linear    = 0.027f,
            .quadratic = 0.0028f
        },
        Entity::PointLight {
            .position  = glm::vec3( 10.0f, 3.0f, -1.0f ),
            .color     = glm::vec3( 1.0f, 1.0f, 1.0f ),
            .intensity = 3.0f,
            .linear    = 0.027f,
            .quadratic = 0.0028f
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

    CommandPool::Instance()->WaitAll();
}

void Rhi::Renderer::Destroy( void ) {
    vkDeviceWaitIdle( Device::Instance()->GetDevice() );

    Core::JobSystem::Instance()->Destroy();
    GUI::Renderer::Instance()->Destroy();
    ShaderManager::Instance()->Destroy();
    PipelineFactory::Instance()->Destroy();
    Timeline::Instance()->Destroy();
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
    CommandList * cmdlist = CommandPool::Instance()->AcquireCommandList();
    currentSwapchain = Swapchain::Instance()->AcquireImage();

    VmaTotalStatistics stats;
    vmaCalculateStatistics( Device::Instance()->GetVMA(), &stats );
    RenderStats::Instance()->vRamUsedGB = stats.total.statistics.allocationBytes / ( 1024.0f * 1024.0f * 1024.0f );
    RenderStats::Instance()->totalVertices = mSponza.GetVertexCount() + mCurtains.GetVertexCount();
    RenderStats::Instance()->renderResolution = mRenderResolution;

    Descriptors::Instance()->UpdateDescriptorSets();
    cmdlist->BeginRendering( currentSwapchain, depthBuffer );
        struct PushConstants {
            glm::mat4 viewProj;
            ulong     lightBuffer;
            uint      lightCount;      uint _pad1;
            glm::vec3 cameraPosition;  uint _pad0;
            ulong     transformBuffer;
            ulong     drawParamBuffer;
        };

        PushConstants pc1 = {
            .viewProj         = mProjection * view,
            .lightBuffer      = Device::Instance()->DeviceAddress( lightBuffer ),
            .lightCount       = lightCount,
            .cameraPosition   = cameraPosition,
            .transformBuffer  = Device::Instance()->DeviceAddress( mSponza.mTransformBuffer ),
            .drawParamBuffer  = Device::Instance()->DeviceAddress( mSponza.mDrawParamBuffer ),
        };
        static_assert( sizeof( PushConstants ) <= 128 );

        cmdlist->BeginDebugLabel( "Sponza Opaque", { 1.0f, 0.0f, 1.0f, 1.0f } );
            cmdlist->BindVertexBuffer( mSponza.mVertexBuffer );
            cmdlist->BindIndexBuffer( mSponza.mIndexBuffer );
            cmdlist->BindRenderPipeline( pipelineOpaque );
            cmdlist->PushConstants( &pc1, sizeof( PushConstants ) );
            cmdlist->DrawIndexedIndirect( mSponza.mOpaqueIndirectBuffer, mSponza.GetOpaqueMeshCount() );
        cmdlist->EndDebugLabel();

        PushConstants pc2 = {
            .viewProj        = pc1.viewProj,
            .lightBuffer     = Device::Instance()->DeviceAddress( lightBuffer ),
            .lightCount      = lightCount,
            .cameraPosition  = cameraPosition,
            .transformBuffer = Device::Instance()->DeviceAddress( mCurtains.mTransformBuffer ),
            .drawParamBuffer = Device::Instance()->DeviceAddress( mCurtains.mDrawParamBuffer )
        };

        cmdlist->BeginDebugLabel( "Sponza Curtains", { 0.0f, 0.0f, 1.0f, 1.0f } );
            cmdlist->BindVertexBuffer( mCurtains.mVertexBuffer );
            cmdlist->BindIndexBuffer( mCurtains.mIndexBuffer );
            cmdlist->BindRenderPipeline( pipelinePlane );
            cmdlist->PushConstants( &pc2, sizeof( PushConstants ) );
            cmdlist->DrawIndexedIndirect( mCurtains.mOpaqueIndirectBuffer, mCurtains.GetOpaqueMeshCount() );
        cmdlist->EndDebugLabel();

        if ( Input::KeyboardInputs::Instance()->GetKey( Input::Key_G ) ) {
            cmdlist->BeginDebugLabel( "GUI", { 0.0f, 1.0f, 0.0f, 1.0f } );
            GUI::Renderer::Instance()->Render( cmdlist );
            cmdlist->EndDebugLabel();
        }
    cmdlist->EndRendering();

    CommandPool::Instance()->Submit( cmdlist, currentSwapchain );
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
    printf( "[DEBUG] Texture                size: %llu\n", sizeof( Texture ) );
    printf( "[DEBUG] TextureMetadata        size: %llu\n", sizeof( TextureMetadata ) );
    printf( "[DEBUG] Buffer                 size: %llu\n", sizeof( Buffer ) );
    printf( "[DEBUG] BufferMetadata         size: %llu\n", sizeof( BufferMetadata ) );
    printf( "[DEBUG] Sampler                size: %llu\n", sizeof( Sampler ) );
    printf( "[DEBUG] SamplerMetadata        size: %llu\n", sizeof( SamplerMetadata ) );
    printf( "[DEBUG] RenderPipeline         size: %llu\n", sizeof( RenderPipeline ) );
    printf( "[DEBUG] RenderPipelineMetadata size: %llu\n", sizeof( RenderPipelineMetadata) );
    printf( "[DEBUG] Shader                 size: %llu\n", sizeof( Shader ) );
    printf( "[DEBUG] ShaderMetadata         size: %llu\n", sizeof( ShaderMetadata ) );
}
