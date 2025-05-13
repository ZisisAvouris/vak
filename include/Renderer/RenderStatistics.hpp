#pragma once
#include <Util/Singleton.hpp>
#include <Util/Defines.hpp>

namespace Rhi {

    class RenderStats final : public Core::Singleton<RenderStats> {
    public:
        void Reset( void ) {
            cpuDrawCalls      = 0;
            indirectDrawCalls = 0;
            totalVertices     = 0;
        }

        float fps;
        uint  cpuDrawCalls;
        uint  indirectDrawCalls;
        float vRamUsedGB;
        uint  totalVertices;
        uint2 renderResolution;

    };

}

