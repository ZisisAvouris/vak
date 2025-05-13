#pragma once
#include <Util/Defines.hpp>
#include <Util/Pool.hpp>

#include <utility>
#include <fstream>
#include <assert.h>
#include <span>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
    
    struct ShaderFile final {
        uint * byteCode = nullptr;
        ulong  size     = 0;
    };
    ShaderFile LoadShader( const std::string & );

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
        Util::BufferHandle mDrawParamBuffer;
        Util::BufferHandle mMaterialBuffer;

        Util::BufferHandle mOpaqueIndirectBuffer;
        Util::BufferHandle mTransparentIndirectBuffer;

        uint GetMeshCount( void ) const { return mMeshCount; }
        uint GetOpaqueMeshCount( void ) const { return mOpaqueCount; }
        uint GetTransparentMeshCount( void ) const { return mTransparentCount; }
        uint GetVertexCount( void ) const { return mVertexCount; }

        void SetTransform( glm::mat4 transform ) { mTransform = transform; }

    private:
        vector<Util::TextureHandle> mTextures;
        glm::mat4 mTransform = glm::mat4( 1.0f );

        std::string mFilepath;
        vector<pair<uint, std::string>> mTextureFilenames;

        uint mMeshCount, mOpaqueCount, mTransparentCount, mVertexCount;
        void UploadTextures( span<DrawParameters> );
        void GetTransformMatrices( const aiNode *, const aiScene *, const glm::mat4 &, span<glm::mat4> );

    };

}
