#pragma once
#include <Util/Singleton.hpp>
#include <Renderer/CommandPool.hpp>
#include <Renderer/RenderBase.hpp>

#define IMGUI_IMPL_VULKAN_NO_PROTOTYPE
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>

namespace GUI {

    class Renderer final : public Core::Singleton<GUI::Renderer> {
    public:
        void Init( void * );
        void Destroy( void );

        void Render( Rhi::CommandList * );

    private:
    };

}
