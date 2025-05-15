#pragma once
#include <Util/Singleton.hpp>
#include <Util/Defines.hpp>
#include <Util/Pool.hpp>
#include <Renderer/RenderBase.hpp>

namespace Rhi {

    enum : _byte {
        Binding_Textures,
        Binding_Samplers,
        Binding_MAX
    };

    class Descriptors final : public Core::Singleton<Descriptors> {
    public:
        void Init( void );
        void Destroy( void );

        void SetUpdateDescriptors( void ) { mShouldUpdateDescriptors = true; }
        void UpdateDescriptorSets( void );

        VkDescriptorSetLayout GetDescriptorSetLayout( void ) const { return mDescriptorLayout; }
        VkDescriptorSet       GetDescriptorSet( void ) const { return mDescriptorSet; }

    private:
        VkDescriptorPool      mDescriptorPool          = VK_NULL_HANDLE;
        VkDescriptorSet       mDescriptorSet           = VK_NULL_HANDLE;
        VkDescriptorSetLayout mDescriptorLayout        = VK_NULL_HANDLE;
        bool                  mShouldUpdateDescriptors = true;

        static constexpr ushort sMaxTextures = 128;
        static constexpr ushort sMaxSamplers = 8;
    };
}
