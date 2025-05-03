#pragma once

namespace Core {

    template<typename Type> class Singleton {
    public:
        static Type * Instance() {
            static Type instance;
            return &instance;
        }

        Singleton( const Singleton * ) = delete;
        Singleton * operator =( const Singleton * ) = delete;
        
    protected:
        Singleton() = default;
    };

}
