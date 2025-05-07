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

    struct Mesh final {
    public:
        Mesh( const aiMesh *, uint );
        ~Mesh() = default;

        Mesh( Mesh && ) = default;
        Mesh & operator=( Mesh&& ) = default;

        Mesh( const Mesh & ) = delete;
        Mesh & operator=( const Mesh & ) = delete;

        uint GetIndexCount( void ) const { return mIndexCount; }
        
    private:
        friend class Model;
        Util::BufferHandle mVertexHandle;
        Util::BufferHandle mIndexHandle;
        glm::mat4          mTransform;
        uint               mTextureId;
        uint               mIndexCount;

    };

    class Model final {
    public:
        Model() = default;
        ~Model() = default;

        bool LoadModelFromFile( const std::string &, uint extraAssimpFlags = 0 );

        Util::BufferHandle GetVertexBuffer( uint index ) const { return mMeshes[index].mVertexHandle; }
        Util::BufferHandle GetIndexBuffer( uint index ) const { return mMeshes[index].mIndexHandle; }

        uint GetTextureId( uint index ) const { return mMeshes[index].mTextureId; }
        uint GetIndexCount( uint index ) const { return mMeshes[index].mIndexCount; }

        uint GetMeshCount( void ) const { return static_cast<uint>( mMeshes.size() ); }

        glm::mat4 & TransformRef( uint index ) { return mMeshes[index].mTransform; }
    
    private:
        vector<Mesh>                    mMeshes;
        vector<Util::TextureHandle>     mTextures;
        vector<pair<uint, std::string>> mTextureFilenames;
        std::string                     mFilepath;

        void UploadTextures( void );
        
    };

}
