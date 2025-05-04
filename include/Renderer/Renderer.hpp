#pragma once
#include <Util/Singleton.hpp>
#include <Util/Containers.hpp>
#include <Util/Pool.hpp>
#include <Renderer/RenderBase.hpp>

#include <Resource/Resource.hpp>

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Rhi {
    using std::vector;
    using namespace Resource;
    
    class Renderer final : public Core::Singleton<Renderer> {
    public:
        void Init( uint2 );
        void Destroy( void );

        void Resize( uint2 );
        void Render( glm::mat4, float );

        bool IsReady( void ) const { return mIsReady; }

        void Delete( Util::TextureHandle );

    private:        
        uint2 mRenderResolution;
        bool  mIsReady = false;

        VkShaderModule smVert, smFrag;
        RenderPipeline opaquePipeline;
        Shader         vertexShader;
        Shader         fragmentShader;
        Util::BufferHandle vertexBuffer;
        Util::BufferHandle indexBuffer;

        VkSampler sampler;
        Image                texImage;
        Util::TextureHandle  texture;
        Util::TextureHandle  depthBuffer;

        void DebugPrintStructSizes( void );

    };
}
