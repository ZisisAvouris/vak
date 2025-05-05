#include <Resource/Resource.hpp>
#include <Renderer/Device.hpp>

namespace Resource {
    using namespace std;

    Shader LoadShader( const string & filename ) {
        ifstream ifs( filename, ios::binary | ios::ate );
        assert( ifs );
        
        Shader result;

        const auto end = ifs.tellg();
        ifs.seekg( 0, ios::beg );
        result.size = static_cast<ulong>( end - ifs.tellg() );

        result.byteCode = new uint[result.size / 4];
        assert( result.byteCode );

        assert( ifs.read( reinterpret_cast<char *>( result.byteCode ), result.size ).good() );
        return result;
    }

    Image LoadTexture( const string & filename ) {
        Image result;
        int width, height, comp;
        result.data = stbi_load( filename.c_str(), &width, &height, &comp, 4 );
        
        assert( result.data );

        result.width = static_cast<ushort>( width );
        result.height = static_cast<ushort>( height );
        result.components = static_cast<_byte>( comp );

        return result;
    }

    Resource::Mesh::Mesh( const aiMesh * mesh, uint index ) {
        vector<Vertex> vertexBuffer;
        vector<uint> indexBuffer;
    
        vertexBuffer.reserve( mesh->mNumVertices );
        for ( uint i = 0; i < mesh->mNumVertices; ++i ) {
            const aiVector3D pos = mesh->mVertices[i];
            const aiVector3D uv  = mesh->mTextureCoords[0][i];
            vertexBuffer.emplace_back( Vertex { .position = glm::vec3( pos.x, pos.y, pos.z ), .uv = glm::vec2( uv.x, uv.y ) } );
        }
    
        mVertexHandle = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( vertexBuffer.size() * sizeof(Vertex) ),
            .ptr       = vertexBuffer.data(),
            .debugName = "Mesh " + std::to_string( index ) + ": Vertex Buffer"
        });
    
        indexBuffer.reserve( mesh->mNumFaces * 3 );
        for ( uint i = 0; i < mesh->mNumFaces; ++i ) {
            indexBuffer.emplace_back( mesh->mFaces[i].mIndices[0] );
            indexBuffer.emplace_back( mesh->mFaces[i].mIndices[1] );
            indexBuffer.emplace_back( mesh->mFaces[i].mIndices[2] );
        }
    
        mIndexHandle = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( indexBuffer.size() * sizeof(uint) ),
            .ptr       = indexBuffer.data(),
            .debugName = "Mesh " + std::to_string( index ) + ": Index Buffer"
        });
    
        mTransform  = glm::mat4( 1.0f );
        mTextureId  = mesh->mMaterialIndex;
        mIndexCount = mesh->mNumFaces * 3;
    }

    bool Resource::Model::LoadModelFromFile( const std::string & filename, uint extraAssimpFlags ) {
        const aiScene * scene = aiImportFile( filename.c_str(), aiProcess_Triangulate | extraAssimpFlags );
        if ( !scene || !scene->HasMeshes() ) {
            printf( "[ERROR] Unable to load model (%s): %s\n", filename.c_str(), aiGetErrorString() );
            return false;
        }
        size_t lastSlashPos = filename.find_last_of("/\\");
        if ( lastSlashPos != std::string::npos ) mFilepath = filename.substr(0, lastSlashPos + 1 );
        else                                     return false;
    
        mMeshes.reserve( scene->mNumMeshes );
        for ( uint i = 0; i < scene->mNumMeshes; ++i ) {
            const aiMesh * mesh = scene->mMeshes[i];
            if ( !mesh->HasTextureCoords( 0 ) ) 
                assert( false && "TODO: Implement no Textures for models" );
            mMeshes.emplace_back( mesh, i );
    
            const aiMaterial * material = scene->mMaterials[mesh->mMaterialIndex];
    
            aiString path;
            if ( material->GetTexture( aiTextureType_DIFFUSE, 0, &path ) == AI_SUCCESS ) {
                auto iter = std::find_if( mTextureFilenames.begin(), mTextureFilenames.end(), [materialIdx = mesh->mMaterialIndex] ( const auto & p ) {
                    return p.first == materialIdx;
                });
    
                if ( iter == mTextureFilenames.end() )
                    mTextureFilenames.push_back( { mesh->mMaterialIndex, std::string( path.C_Str() ) } );
            }
        }

        UploadTextures();

        aiReleaseImport( scene );
        return true;
    }

    void Resource::Model::UploadTextures( void ) {
        mTextures.resize( mTextureFilenames.size() );
        
        vector<bool> updated( mMeshes.size(), false );
        for ( uint i = 0; i < mTextureFilenames.size(); ++i ) {
            const auto & texPair = mTextureFilenames[i];
    
            std::string texturePath = mFilepath + texPair.second;
            Image img = LoadTexture( texturePath );
    
            mTextures[i] = Rhi::Device::Instance()->CreateTexture({
                .type      = VK_IMAGE_TYPE_2D,
                .format    = VK_FORMAT_R8G8B8A8_UNORM,
                .extent    = { static_cast<uint>( img.width ), static_cast<uint>( img.height ), 1 },
                .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
                .data      = img.data,
                .debugName = texPair.second 
            });
    
            for ( uint j = 0; j < mMeshes.size(); ++j ) {
                if ( mMeshes[j].mTextureId == texPair.first && !updated[j] ) {
                    mMeshes[j].mTextureId = mTextures[i].Index();
                    updated[j]            = true;
                }
            }
    
            stbi_image_free( img.data );
        }
    }

}
