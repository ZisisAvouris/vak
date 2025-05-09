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

    Resource::Mesh::~Mesh() {
        // @todo: when scene editing is implemented, re-enable this and fix the bug in the Pool implementation
        // Rhi::Device::Instance()->Delete( mVertexBuffer );
        // Rhi::Device::Instance()->Delete( mIndexBuffer );
        // Rhi::Device::Instance()->Delete( mIndirectBuffer );
        // Rhi::Device::Instance()->Delete( mMaterialBuffer );
        // Rhi::Device::Instance()->Delete( mTransformBuffer );
        // Rhi::Device::Instance()->Delete( mDrawParamBuffer );
    }

    bool Resource::Mesh::LoadMeshFromFile( const std::string & filename, uint extraAssimpFlags ) {
        const aiScene * scene = aiImportFile( filename.c_str(), aiProcess_Triangulate | extraAssimpFlags );
        if ( !scene || !scene->HasMeshes() ) {
            printf( "[ERROR] Unable to load model (%s): %s\n", filename.c_str(), aiGetErrorString() );
            return false;
        }
        size_t lastSlashPos = filename.find_last_of("/\\");
        if ( lastSlashPos != std::string::npos ) mFilepath = filename.substr(0, lastSlashPos + 1 );
        else                                     return false;

        uint totalVertices = 0, totalIndices = 0;
        for ( uint i = 0; i < scene->mNumMeshes; ++i ) {
            totalVertices += scene->mMeshes[i]->mNumVertices;
            totalIndices  += scene->mMeshes[i]->mNumFaces * 3;
        }
        mMeshCount = scene->mNumMeshes;

        vector<Vertex> vertexData;           vertexData.reserve( totalVertices );
        vector<uint> indexData;              indexData.reserve( totalIndices );
        vector<VkDrawIndexedIndirectCommand> cmds; cmds.reserve( scene->mNumMeshes );
        vector<DrawParameters> paramData;    paramData.reserve( scene->mNumMeshes );
        vector<glm::mat4> transformData;     transformData.reserve( 1 ); // @todo: properly extract per node transform, may be required for gltf

        uint indexStart = 0, vertexStart = 0;
        for ( uint i = 0; i < scene->mNumMeshes; ++i ) {
            const aiMesh * mesh = scene->mMeshes[i];

            for ( uint vtx = 0; vtx < mesh->mNumVertices; ++vtx ) {
                const aiVector3D pos  = mesh->mVertices[vtx];
                const aiVector3D norm = mesh->mNormals[vtx];
                const aiVector3D uv   = mesh->mTextureCoords[0][vtx];
                vertexData.emplace_back( Vertex {
                    .position = glm::vec3( pos.x, pos.y, pos.z ),
                    .normal   = glm::vec3( norm.x, norm.y, norm.z ),
                    .uv       = glm::vec2( uv.x, uv.y )
                });
            }

            for ( uint idx = 0; idx < mesh->mNumFaces; ++idx ) {
                indexData.emplace_back( mesh->mFaces[idx].mIndices[0] );
                indexData.emplace_back( mesh->mFaces[idx].mIndices[1] );
                indexData.emplace_back( mesh->mFaces[idx].mIndices[2] );
            }

            cmds.emplace_back( VkDrawIndexedIndirectCommand {
                .indexCount    = mesh->mNumFaces * 3,
                .instanceCount = 1,
                .firstIndex    = indexStart,
                .vertexOffset  = (int)vertexStart,
                .firstInstance = i
            });
            paramData.emplace_back( DrawParameters { .transformID = 0, .materialID = mesh->mMaterialIndex } );

            const aiMaterial * material = scene->mMaterials[mesh->mMaterialIndex];
    
            aiString path;
            if ( material->GetTexture( aiTextureType_DIFFUSE, 0, &path ) == AI_SUCCESS ) {
                auto iter = std::find_if( mTextureFilenames.begin(), mTextureFilenames.end(), [materialIdx = mesh->mMaterialIndex] ( const auto & p ) {
                    return p.first == materialIdx;
                });
    
                if ( iter == mTextureFilenames.end() )
                    mTextureFilenames.push_back( { mesh->mMaterialIndex, std::string( path.C_Str() ) } );
            }

            vertexStart += mesh->mNumVertices;
            indexStart  += mesh->mNumFaces * 3;
        }
        UploadTextures( paramData );

        transformData.emplace_back( glm::mat4( 1.0f ) );

        mVertexBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( vertexData.size() * sizeof( Vertex ) ),
            .ptr       = vertexData.data(),
            .debugName = filename + " Vertex Data"
        });
        mIndexBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( indexData.size() * sizeof( uint ) ),
            .ptr       = indexData.data(),
            .debugName = filename + " Index Data"
        });
        mTransformBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( transformData.size() * sizeof( glm::mat4 ) ),
            .ptr       = transformData.data(),
            .debugName = filename + " Transform Data"
        });
        mDrawParamBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( paramData.size() * sizeof( DrawParameters ) ),
            .ptr       = paramData.data(),
            .debugName = filename + " Draw Parameters"
        });
        mIndirectBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( cmds.size() * sizeof( VkDrawIndexedIndirectCommand ) ),
            .ptr       = cmds.data(),
            .debugName = filename + " Indirect Commands"
        });

        aiReleaseImport( scene );
        return true;
    }

    void Resource::Mesh::UploadTextures( span<DrawParameters> drawParams ) {
        mTextures.resize( mTextureFilenames.size() );
        
        vector<bool> updated( mMeshCount, false );
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
    
            for ( uint j = 0; j < mMeshCount; ++j ) {
                if ( drawParams[j].materialID == texPair.first && !updated[j] ) {
                    drawParams[j].materialID = mTextures[i].Index();
                    updated[j]               = true;
                }
            }
    
            stbi_image_free( img.data );
        }
    }

}
