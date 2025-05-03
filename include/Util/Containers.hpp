#pragma once
#include <Util/Defines.hpp>

template<typename Type> struct Vec2 {
    Type x;
    Type y;

    Vec2<Type>& operator=(const Vec2<Type>& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
        }
        return *this;
    }

    inline bool operator ==( const Vec2<Type> &other ) const { return x == other.x && y == other.y; }
};

using uint2 = Vec2<uint>;
using float2 = Vec2<float>;
