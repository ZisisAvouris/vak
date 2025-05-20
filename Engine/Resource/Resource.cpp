#include <Resource/Resource.hpp>
#include <Renderer/Device.hpp>
#include <Core/JobSystem.hpp>

namespace Resource {
    using namespace std;

    ShaderFile LoadShader( const string & filename ) {
        ifstream ifs( filename, ios::binary | ios::ate );
        assert( ifs );

        ShaderFile result;

        const auto end = ifs.tellg();
        ifs.seekg( 0, ios::beg );
        result.size = static_cast<ulong>( end - ifs.tellg() );

        result.byteCode = new uint[result.size / 4];
        assert( result.byteCode );

        const bool ok = ifs.read( reinterpret_cast<char *>( result.byteCode ), result.size ).good();
        if ( !ok )
            exit(1);
        return result;
    }

    ktxTexture2 * LoadTexture( const fs::path & path, bool shouldCompress ) {
        const string filename   = path.stem().string();
        const string cacheDir   = (*path.begin()).string() + "/.cache/";
        const string compressed = cacheDir + filename + ".bc7";

        ktxTexture2 * texture;
        // check cache for compressed texture, else continue to loading and compressing
        if ( shouldCompress && fs::exists( compressed ) ) {
            ktxResult loadOK = ktxTexture2_CreateFromNamedFile( compressed.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture );
            if ( loadOK != KTX_SUCCESS ) {
                printf("Loading %s from cache failed %u\n", compressed.c_str(), loadOK);
                return nullptr;
            }
            return texture;
        }

        ktxTextureCreateInfo ci = {
            .vkFormat        = VK_FORMAT_R8G8B8A8_UNORM,
            .baseDepth       = 1,
            .numDimensions   = 2,
            .numLevels       = 1,
            .numLayers       = 1,
            .numFaces        = 1,
            .generateMipmaps = KTX_FALSE
        };

        int x, y;
        _byte * data = stbi_load( path.string().c_str(), &x, &y, nullptr, 4 );
        if ( !data ) {
            printf("Could not load image %s: %s\n", path.string().c_str(), stbi_failure_reason());
            return nullptr;
        }
        ci.baseWidth  = (uint)x;
        ci.baseHeight = (uint)y;

        uint mipCount = 1; // Original texture + the resulting mips
        while ( x > 512 && y > 512 ) {
            x = x >> 1;
            y = y >> 1;
            mipCount++;
        }
        ci.numLevels = mipCount;

        ktxResult createOK = ktxTexture2_Create( &ci, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture );
        if ( createOK != KTX_SUCCESS ) {
            printf("KTX create failed %u\n", createOK );
            stbi_image_free( data );
            return nullptr;
        }
        memcpy( ktxTexture_GetData(ktxTexture(texture)), data, ci.baseWidth * ci.baseHeight * 4 );

        for ( uint i = 0; i < ci.numLevels; ++i ) {
            const uint mipw = std::max<uint>( 1u, ci.baseWidth >> i );
            const uint miph = std::max<uint>( 1u, ci.baseHeight >> i );

            const ktx_size_t mipsize = ktxTexture_GetLevelSize( ktxTexture(texture), i );
            ktx_size_t mipOffset;
            ktxTexture2_GetImageOffset( texture, i, 0, 0, &mipOffset );

            _byte * dst = ktxTexture_GetData( ktxTexture(texture) ) + mipOffset;
            stbir_resize_uint8_linear( data, ci.baseWidth, ci.baseHeight, 0, dst, mipw, miph, 0, STBIR_RGBA );
        }

        if ( shouldCompress ) {
            if ( x % 4 != 0 || y % 4 != 0 ) {
                printf("Texture %s extent is not divisible by 4!\n", path.string().c_str());
                stbi_image_free( data );
                return nullptr;
            }

            ktxResult compressionOK = ktxTexture2_CompressBasis( texture, 255 );
            if ( compressionOK != KTX_SUCCESS ) {
                printf("Texture %s compression failed %u\n", path.string().c_str(), compressionOK);
                stbi_image_free(data);
                return nullptr;
            }
            ktxResult transcodeOK = ktxTexture2_TranscodeBasis( texture, KTX_TTF_BC7_RGBA, 0 );
            if ( transcodeOK != KTX_SUCCESS ) {
                printf("Texture %s transcode to BC7 failed %u\n", path.string().c_str(), transcodeOK);
                stbi_image_free(data);
                return nullptr;
            }

            // Cache compressed texture for next runs
            ktxTexture2_WriteToNamedFile( texture, compressed.c_str() );
            texture->vkFormat = VK_FORMAT_BC7_UNORM_BLOCK;
        }
        stbi_image_free( data );
        return texture;
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

    bool Resource::Mesh::LoadMeshFromFile( const fs::path & path, bool compressTextures, uint extraAssimpFlags ) {
        const aiScene * scene = aiImportFile( path.string().c_str(), aiProcess_Triangulate | aiProcess_CalcTangentSpace | extraAssimpFlags );
        if ( !scene || !scene->HasMeshes() ) {
            printf( "[ERROR] Unable to load model (%s): %s\n", path.string().c_str(), aiGetErrorString() );
            return false;
        }
        fs::path parentPath = path.parent_path().string() + "/";

        uint totalVertices = 0, totalIndices = 0, totalOpaqueMeshes = 0, totalTransparentMeshes = 0;
        for ( uint i = 0; i < scene->mNumMeshes; ++i ) {
            totalVertices += scene->mMeshes[i]->mNumVertices;
            totalIndices  += scene->mMeshes[i]->mNumFaces * 3;

            const aiMaterial * material = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];

            float opacity = 1.0f;
            if ( material->Get( AI_MATKEY_OPACITY, opacity ) == AI_SUCCESS ) {
                totalOpaqueMeshes += ( opacity == 1.0f );
                mVertexCount += ( opacity == 1.0f ) * scene->mMeshes[i]->mNumVertices;
                totalTransparentMeshes += ( opacity < 1.0f );
            }

            aiString texturePath;
            if ( material->GetTexture( aiTextureType_DIFFUSE, 0, &texturePath ) == AI_SUCCESS ) {
                if ( !mTextureIdMap.contains( texturePath.C_Str() ) )
                    mTextureIdMap[texturePath.C_Str()] = scene->mMeshes[i]->mMaterialIndex;
            }
            if ( material->GetTexture( aiTextureType_NORMALS, 0, &texturePath ) == AI_SUCCESS ) {
                if ( !mTextureIdMap.contains( texturePath.C_Str() ) )
                    mTextureIdMap[texturePath.C_Str()] = scene->mMeshes[i]->mMaterialIndex;
            }
            if ( material->GetTexture( aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath ) == AI_SUCCESS ) {
                if ( !mTextureIdMap.contains( texturePath.C_Str() ) )
                    mTextureIdMap[texturePath.C_Str()] = scene->mMeshes[i]->mMaterialIndex;
            }
        }
        mMeshCount = totalOpaqueMeshes + totalTransparentMeshes;
        mOpaqueCount = totalOpaqueMeshes;
        mTransparentCount = totalTransparentMeshes;

        std::mutex mapMutex;
        for ( const auto & [path, _] : mTextureIdMap ) {
            Core::JobSystem::Instance()->DispatchJob( [&] {
                ktxTexture2 * texture = LoadTexture( parentPath.string() + path, compressTextures );
                Util::TextureHandle handle = Rhi::Device::Instance()->CreateTexture( texture, path );
                ktxTexture2_Destroy( texture );

                std::lock_guard<std::mutex> lock( mapMutex );
                mTextureIdMap[path] = handle.Index();
            });
        }

        vector<Vertex> vertexData;
        vertexData.reserve( totalVertices );
        vector<uint> indexData;
        indexData.reserve( totalIndices );
        vector<VkDrawIndexedIndirectCommand> opaqueCmds;
        opaqueCmds.reserve( totalOpaqueMeshes );
        vector<DrawParameters> paramData;
        paramData.reserve( scene->mNumMeshes );
        vector<glm::mat4> transformData;
        transformData.resize( scene->mNumMeshes );

        uint indexStart = 0, vertexStart = 0, opaqueIndex = 0, transparentIndex = 0;
        for ( uint i = 0; i < scene->mNumMeshes; ++i ) {
            const aiMesh * mesh = scene->mMeshes[i];

            for ( uint vtx = 0; vtx < mesh->mNumVertices; ++vtx ) {
                const aiVector3D pos  = mesh->mVertices[vtx];
                const aiVector3D norm = mesh->mNormals[vtx];
                const aiVector3D tang = mesh->mTangents[vtx];
                const aiVector3D uv   = mesh->mTextureCoords[0][vtx];
                vertexData.emplace_back( Vertex {
                    .position = glm::vec3( pos.x, pos.y, pos.z ),
                    .normal   = glm::packSnorm3x10_1x2( { norm.x, norm.y, norm.z, 0.0f } ),
                    .tangent  = glm::packSnorm3x10_1x2( { tang.x, tang.y, tang.z, 0.0f } ),
                    .uv       = glm::packHalf2x16( { uv.x, uv.y } )
                });
            }

            for ( uint idx = 0; idx < mesh->mNumFaces; ++idx ) {
                indexData.emplace_back( mesh->mFaces[idx].mIndices[0] );
                indexData.emplace_back( mesh->mFaces[idx].mIndices[1] );
                indexData.emplace_back( mesh->mFaces[idx].mIndices[2] );
            }

            const aiMaterial * material = scene->mMaterials[mesh->mMaterialIndex];

            float opacity = 1.0f;
            if ( material->Get( AI_MATKEY_OPACITY, opacity ) == AI_SUCCESS ) {
                if ( opacity == 1.0f ) {
                    opaqueCmds.emplace_back( VkDrawIndexedIndirectCommand {
                        .indexCount    = mesh->mNumFaces * 3,
                        .instanceCount = 1,
                        .firstIndex    = indexStart,
                        .vertexOffset  = (int)vertexStart,
                        .firstInstance = i
                    });
                }
            }
            paramData.emplace_back( DrawParameters { .transformID = i  } );

            vertexStart += mesh->mNumVertices;
            indexStart  += mesh->mNumFaces * 3;
        }
        GetTransformMatrices( scene->mRootNode, scene, mTransform, transformData );
        Core::JobSystem::Instance()->WaitAll();

        mVertexBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( vertexData.size() * sizeof( Vertex ) ),
            .ptr       = vertexData.data(),
            .debugName = path.string() + " Vertex Data"
        });
        mIndexBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( indexData.size() * sizeof( uint ) ),
            .ptr       = indexData.data(),
            .debugName = path.string() + " Index Data"
        });
        mTransformBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( transformData.size() * sizeof( glm::mat4 ) ),
            .ptr       = transformData.data(),
            .debugName = path.string() + " Transform Data"
        });
        mOpaqueIndirectBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( opaqueCmds.size() * sizeof( VkDrawIndexedIndirectCommand ) ),
            .ptr       = opaqueCmds.data(),
            .debugName = path.string() + " Opaque Indirect Commands"
        });

        for ( uint i = 0; i < scene->mNumMeshes; ++i ) {
            const aiMesh     * mesh     = scene->mMeshes[i];
            const aiMaterial * material = scene->mMaterials[mesh->mMaterialIndex];

            aiString texturePath;
            if ( material->GetTexture( aiTextureType_DIFFUSE, 0, &texturePath ) == AI_SUCCESS ) {
                paramData[i].baseColorID = mTextureIdMap[texturePath.C_Str()];
            }
            if ( material->GetTexture( aiTextureType_NORMALS, 0, &texturePath ) == AI_SUCCESS ) {
                paramData[i].normalID = mTextureIdMap[texturePath.C_Str()];
            }
            if ( material->GetTexture( aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath ) == AI_SUCCESS ) {
                paramData[i].metallicRoughnessID = mTextureIdMap[texturePath.C_Str()];
            }
        }

        mDrawParamBuffer = Rhi::Device::Instance()->CreateBuffer({
            .usage     = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            .storage   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .size      = static_cast<uint>( paramData.size() * sizeof( DrawParameters ) ),
            .ptr       = paramData.data(),
            .debugName = path.string() + " Draw Parameters"
        });

        aiReleaseImport( scene );
        mTextureIdMap.clear();
        return true;
    }

    void Resource::Mesh::GetTransformMatrices( const aiNode * node, const aiScene * scene, const glm::mat4 & parentTransform, span<glm::mat4> transformBuffer ) {
        glm::mat4 localTransform  = glm::transpose( glm::make_mat4( &node->mTransformation.a1 ) );
        glm::mat4 globalTransform = parentTransform * localTransform;

        for ( uint i = 0; i < node->mNumMeshes; ++i )
            transformBuffer[node->mMeshes[i]] = globalTransform;

        for ( uint i = 0; i < node->mNumChildren; ++i )
            GetTransformMatrices( node->mChildren[i], scene, globalTransform, transformBuffer );
    }

}
