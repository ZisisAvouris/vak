#pragma once
#include <Util/Singleton.hpp>
#include <Util/Pool.hpp>
#include <Renderer/RenderBase.hpp>

namespace Rhi {

    using ShaderPool = Util::Pool<Util::_Shader, Shader, ShaderMetadata>;

    class ShaderManager final : public Core::Singleton<ShaderManager> {
    public:
        void Init( void ) {}
        void Destroy( void );

        Util::ShaderHandle LoadShader( const ShaderSpecification & );
        void Delete( Util::ShaderHandle );

        ShaderPool * GetShaderPool( void ) { return &mShaderPool; }

    private:
        ShaderPool mShaderPool { 8, "Shader" };
    };
}
