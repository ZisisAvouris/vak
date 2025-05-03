#pragma once
#include <Util/Singleton.hpp>
#include <Util/Defines.hpp>
#include <Renderer/RenderBase.hpp>

#include <vector>
#include <span>
#include <assert.h>

namespace Rhi {
    using namespace std;
    
    class RenderContext final : public Core::Singleton<RenderContext> {
    public:
        void Init( void );
        void Destroy( void );

        VkInstance GetVulkanInstance( void ) const { return mInstance; }

    private:
        VkInstance               mInstance;
        VkDebugUtilsMessengerEXT mDebugMessenger;

        bool mEnableValidation = false;
        
        bool HasExtension( const char *, span<const VkExtensionProperties> );
    };
}
