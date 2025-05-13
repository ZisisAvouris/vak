#include <Renderer/Shader.hpp>
#include <Renderer/Device.hpp>

void Rhi::ShaderManager::Destroy( void ) {
    for ( uint i = 0; i < mShaderPool.GetObjectCount(); ++i ) {
        Util::ShaderHandle handle = mShaderPool.GetHandle( i );
        if ( handle.Valid() ) Delete( handle );
    }
}

Util::ShaderHandle Rhi::ShaderManager::LoadShader( const ShaderSpecification & spec ) {
    Shader shader;
    ShaderMetadata metadata = { .spec = spec };
    metadata.debugName += spec.debugName;

    ShaderFile file = Resource::LoadShader( spec.filename );

    VkShaderModuleCreateInfo smci = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = file.size,
        .pCode    = file.byteCode,
    };
    vkCreateShaderModule( Device::Instance()->GetDevice(), &smci, nullptr, &shader.sm );
    delete[] file.byteCode;

    return mShaderPool.Create( std::move( shader ), std::move( metadata ) );
}

void Rhi::ShaderManager::Delete( Util::ShaderHandle handle ) {
    Shader * shader = mShaderPool.Get( handle );
    if ( !shader ) assert( false );
    vkDestroyShaderModule( Device::Instance()->GetDevice(), shader->sm, nullptr );
}
