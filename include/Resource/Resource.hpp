#pragma once
#include <Util/Defines.hpp>
#include <Util/Pool.hpp>

#include <utility>
#include <fstream>
#include <assert.h>
#include <span>
#include <vector>
#include <map>
#include <filesystem>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <ktx.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

namespace Resource {
    using std::pair;
    using std::span;
    using std::vector;
    using std::map;
    namespace fs = std::filesystem;

    struct Vertex final {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec2 uv;
    };

    struct ShaderFile final {
        uint * byteCode = nullptr;
        ulong  size     = 0;
    };
    ShaderFile LoadShader( const std::string & );

    ktxTexture2 * LoadTexture( const fs::path &, bool );

    struct DrawParameters final {
        uint transformID;
        uint baseColorID;
        uint normalID;
        uint metallicRoughnessID;
    };

    struct Mesh final {
    public:
        Mesh() = default;
        ~Mesh();

        bool LoadMeshFromFile( const fs::path &, bool, uint = 0 );

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

        map<std::string, uint> mTextureIdMap;

        uint mMeshCount, mOpaqueCount, mTransparentCount, mVertexCount;
        void GetTransformMatrices( const aiNode *, const aiScene *, const glm::mat4 &, span<glm::mat4> );

    };

}
