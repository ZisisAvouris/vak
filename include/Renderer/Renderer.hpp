#pragma once
#include <Util/Singleton.hpp>
#include <Util/Containers.hpp>
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
        void Render( float );

        bool IsReady( void ) const { return mIsReady; }

    private:        
        uint2 mRenderResolution;
        bool  mIsReady = false;

        RenderPipeline opaquePipeline;
        Shader         vertexShader;
        Shader         fragmentShader;
        Buffer         vertexBuffer;
        Buffer         indexBuffer;

        Image          texImage;
        Texture        texture;

    };
}
