#pragma once
#include <Util/Defines.hpp>
#include <Util/Singleton.hpp>
#include <Renderer/RenderBase.hpp>

#include <array>

namespace Rhi {

    class PipelineFactory final : public Core::Singleton<PipelineFactory> {
    public:
        void Init( void );
        void Destroy( void );

        RenderPipeline CreateRenderPipeline( const RenderPipelineSpecification & );

        VkPipelineCache GetPipelineCache( void ) const { return mPipelineCache; }

    private:
        VkPipelineCache mPipelineCache;

    };
}
