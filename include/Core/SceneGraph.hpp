#pragma once
#include <Util/Defines.hpp>
#include <Util/Singleton.hpp>

namespace Core {

    class SceneGraph final : public Core::Singleton<SceneGraph> {
    public:
        void Init( void );
        void Destroy( void );

    private:
        
    };
}