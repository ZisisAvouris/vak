#pragma once
#include <Util/Singleton.hpp>
#include <Util/Containers.hpp>
#include <Util/Pool.hpp>
#include <Renderer/RenderBase.hpp>

#include <Entity/Lights.hpp>

#include <Resource/Resource.hpp>

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Rhi {
    using std::vector;
    using namespace Resource;

    class Renderer final : public Core::Singleton<Renderer> {
    public:
        void Init( uint2, void * );
        void Destroy( void );

        void Resize( uint2 );
        void Render( glm::vec3, glm::mat4, float );

        bool IsReady( void ) const { return mIsReady; }

        void Delete( Util::TextureHandle );

    private:
        uint2 mRenderResolution;
        bool  mIsReady = false;

        glm::mat4 mProjection = glm::mat4( 0.0f );
        float mNear = 0.1f, mFar = 100.0f, mFov = 60.0f;

        Util::RenderPipelineHandle pipelineOpaque, pipelinePlane;
        Util::ShaderHandle shMainVert, shMainFrag;
        Util::BufferHandle vertexBuffer;
        Util::BufferHandle indexBuffer;

        Util::SamplerHandle  sampler;
        Util::TextureHandle  depthBuffer, currentSwapchain;

        Util::BufferHandle lightBuffer;
        uint               lightCount;

        Mesh mSponza, mCurtains;

        void ComputeProjectionMatrix( void );
        void DebugPrintStructSizes( void );

    };
}
