#include <Renderer/Descriptors.hpp>
#include <Renderer/Device.hpp>

void Rhi::Descriptors::Init( void ) {
    const VkShaderStageFlags shaderStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutBinding bindings[Binding_MAX] = {
    {
            .binding         = Binding_Textures,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = sMaxTextures,
            .stageFlags      = shaderStages
        },
        {
            .binding         = Binding_Samplers,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = sMaxSamplers,
            .stageFlags      = shaderStages
        }
    };
    const uint flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    VkDescriptorBindingFlags bindingFlags[Binding_MAX] = { flags };

    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlagsCI = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
        .bindingCount  = Binding_MAX,
        .pBindingFlags = bindingFlags
    };
    VkDescriptorSetLayoutCreateInfo setLayoutCI = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = &setLayoutBindingFlagsCI,
        .flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
        .bindingCount = Binding_MAX,
        .pBindings    = bindings
    };
    assert( vkCreateDescriptorSetLayout( Device::Instance()->GetDevice(), &setLayoutCI, nullptr, &mDescriptorLayout ) == VK_SUCCESS );

    VkDescriptorPoolSize poolSizes[Binding_MAX] {
        { .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = sMaxTextures },
        { .type = VK_DESCRIPTOR_TYPE_SAMPLER,       .descriptorCount = sMaxSamplers }
    };
    VkDescriptorPoolCreateInfo poolCI = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets       = 1,
        .poolSizeCount = Binding_MAX - 1,
        .pPoolSizes    = poolSizes
    };
    assert( vkCreateDescriptorPool( Device::Instance()->GetDevice(), &poolCI, nullptr, &mDescriptorPool ) == VK_SUCCESS );

    VkDescriptorSetAllocateInfo ai = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = mDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &mDescriptorLayout
    };
    assert( vkAllocateDescriptorSets( Device::Instance()->GetDevice(), &ai, &mDescriptorSet ) == VK_SUCCESS );
}

void Rhi::Descriptors::Destroy( void ) {
    vkDestroyDescriptorSetLayout( Device::Instance()->GetDevice(), mDescriptorLayout, nullptr );
    vkDestroyDescriptorPool( Device::Instance()->GetDevice(), mDescriptorPool, nullptr );
}

void Rhi::Descriptors::UpdateDescriptorSets( void ) {
    if ( !mShouldUpdateDescriptors )
        return;
    assert( Device::Instance()->GetTexturePool()->GetEntryCount() < sMaxTextures && "Exceeded max number of textures!" );

    vector<VkDescriptorImageInfo> descriptorInfoSampledImages;
    descriptorInfoSampledImages.reserve( Device::Instance()->GetTexturePool()->GetEntryCount() );

    Util::TextureHandle dummy = Device::Instance()->GetTexturePool()->GetHandle( 0 );
    VkImageView dummyView = Device::Instance()->GetTexturePool()->Get( dummy )->view;

    for ( uint i = 0; i < Device::Instance()->GetTexturePool()->GetObjectCount(); ++i ) {
        Util::TextureHandle handle = Device::Instance()->GetTexturePool()->GetHandle( i );
        if ( handle.Valid() ) {
            Texture * tex = Device::Instance()->GetTexturePool()->Get( handle );

            const bool isSampled = ( tex->usage & VK_IMAGE_USAGE_SAMPLED_BIT ) > 0;
            descriptorInfoSampledImages.push_back( VkDescriptorImageInfo {
                .sampler     = VK_NULL_HANDLE,
                .imageView   = isSampled ? tex->view : dummyView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }
    }

    vector<VkDescriptorImageInfo> descriptorInfoSamplers;
    descriptorInfoSamplers.reserve( Device::Instance()->GetSamplerPool()->GetEntryCount() );

    for ( uint i = 0; i < Device::Instance()->GetSamplerPool()->GetObjectCount(); ++i ) {
        Util::SamplerHandle handle = Device::Instance()->GetSamplerPool()->GetHandle( i );
        if ( handle.Valid() ) {
            Sampler * sampler = Device::Instance()->GetSamplerPool()->Get( handle );

            // @todo: ugly hack, fix in Pool implementation
            if ( sampler->sampler == nullptr )
                continue;

            descriptorInfoSamplers.push_back( VkDescriptorImageInfo {
                .sampler     = sampler->sampler,
                .imageView   = VK_NULL_HANDLE,
                .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
            });
        }
    }

    VkWriteDescriptorSet write[Binding_MAX] = {};
    uint numWrites = 0;

    if ( !descriptorInfoSampledImages.empty() )
        write[numWrites++] = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = mDescriptorSet,
            .dstBinding      = Binding_Textures,
            .dstArrayElement = 0,
            .descriptorCount = static_cast<uint>( descriptorInfoSampledImages.size() ),
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo      = descriptorInfoSampledImages.data()
        };
    if ( !descriptorInfoSamplers.empty() )
        write[numWrites++] = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = mDescriptorSet,
            .dstBinding      = Binding_Samplers,
            .dstArrayElement = 0,
            .descriptorCount = static_cast<uint>( descriptorInfoSamplers.size() ),
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo      = descriptorInfoSamplers.data()
        };

    vkUpdateDescriptorSets( Device::Instance()->GetDevice(), numWrites, write, 0, nullptr );
    mShouldUpdateDescriptors = false;
}
