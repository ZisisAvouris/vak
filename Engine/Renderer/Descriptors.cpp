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

void Rhi::Descriptors::UpdateDescriptorSets( Util::TextureHandle handle, VkSampler sampler ) {
    TextureHot * hot = Device::Instance()->GetTexturePool()->GetHot( handle );
    VkDescriptorImageInfo descriptorImageInfo = {
        .sampler     = VK_NULL_HANDLE,
        .imageView   = hot->view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkDescriptorImageInfo descriptorSamplerInfo = {
        .sampler     = sampler,
        .imageView   = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkWriteDescriptorSet write[Binding_MAX] = {
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = mDescriptorSet,
            .dstBinding      = Binding_Textures,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo      = &descriptorImageInfo
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = mDescriptorSet,
            .dstBinding      = Binding_Samplers,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo      = &descriptorSamplerInfo
        }
    };

    vkUpdateDescriptorSets( Device::Instance()->GetDevice(), 2, write, 0, nullptr );
}
