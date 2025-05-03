#pragma once
#include <Util/Defines.hpp>

#include <utility>
#include <fstream>
#include <assert.h>

#include <glm/glm.hpp>

#include <stb_image.h>

namespace Resource {
    using std::pair;

    struct Vertex final {
        glm::vec3 position;
        glm::vec2 uv;
    };
    
    struct Shader final {
        uint * byteCode = nullptr;
        ulong  size     = 0;
    };
    Shader LoadShader( const std::string & );

    struct Image final {
        _byte * data       = nullptr;
        ushort  width      = 0;
        ushort  height     = 0;
        _byte   components = 0;
    };
    Image LoadTexture( const std::string & );

}
