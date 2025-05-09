#pragma once
#include <Util/Defines.hpp>
#include <Util/Pool.hpp>

#include <utility>
#include <fstream>
#include <assert.h>
#include <span>
#include <vector>

#include <glm/glm.hpp>
#include <stb_image.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

namespace Resource {
    using std::pair;
    using std::span;
    using std::vector;

    struct Vertex final {
        glm::vec3 position;
        glm::vec3 normal;
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

    struct DrawParameters final {
        uint transformID;
        uint materialID;
    };

    struct Mesh final {
    public:
        Mesh() = default;
        ~Mesh();

        bool LoadMeshFromFile( const std::string &, uint = 0 );

        Util::BufferHandle mVertexBuffer;
        Util::BufferHandle mIndexBuffer;
        Util::BufferHandle mTransformBuffer;
        Util::BufferHandle mIndirectBuffer;
        Util::BufferHandle mDrawParamBuffer;
        Util::BufferHandle mMaterialBuffer;

        uint GetMeshCount( void ) const { return mMeshCount; }
        
    private:
        vector<Util::TextureHandle> mTextures;

        std::string mFilepath;
        vector<pair<uint, std::string>> mTextureFilenames;

        uint mMeshCount;
        void UploadTextures( span<DrawParameters> );

    };

}
