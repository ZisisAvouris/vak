#pragma once
#include <Util/Defines.hpp>
#include <Util/Singleton.hpp>
#include <Util/Pool.hpp>
#include <Renderer/RenderBase.hpp>

#include <array>

namespace Rhi {

    using RenderPipelinePool = Util::Pool<Util::_RenderPipeline, RenderPipeline, RenderPipelineMetadata>;

    class PipelineFactory final : public Core::Singleton<PipelineFactory> {
    public:
        void Init( void );
        void Destroy( void );

        Util::RenderPipelineHandle CreateRenderPipeline( const RenderPipelineSpecification & );

        void Delete( Util::RenderPipelineHandle );

        VkPipelineCache GetPipelineCache( void ) const { return mPipelineCache; }
        RenderPipelinePool * GetRenderPipelinePool( void ) { return &mRenderPipelinePool; }

    private:
        VkPipelineCache    mPipelineCache;
        RenderPipelinePool mRenderPipelinePool { 4, "Render Pipeline" };

    };
}
