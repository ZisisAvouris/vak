#pragma once
#include <Util/Defines.hpp>
#include <Util/Singleton.hpp>
#include <Renderer/RenderBase.hpp>

namespace Rhi {

    class Timeline final : public Core::Singleton<Timeline> {
    public:
        void Init( void );
        void Destroy( void );

        void SignalTimeline( ulong );

        VkSemaphore GetTimeline( void ) const { return mTimeline; }
        ulong GetCurrentFrame( void ) const { return mFrame; }

        void IncrementFrame( void ) { mFrame++; }

    private:
        VkSemaphore mTimeline;
        ulong       mFrame;
    };
}
