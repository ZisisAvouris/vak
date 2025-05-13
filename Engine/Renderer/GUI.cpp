#include <Renderer/GUI.hpp>
#include <Renderer/Device.hpp>
#include <Renderer/RenderContext.hpp>
#include <Renderer/Pipeline.hpp>
#include <Renderer/Swapchain.hpp>
#include <Core/WindowManager.hpp>
#include <Renderer/RenderStatistics.hpp>

void GUI::Renderer::Init( void * windowHandle ) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplWin32_Init( windowHandle );

    const VkFormat surfaceFormat = Rhi::Swapchain::Instance()->GetSurfaceFormat();
    ImGui_ImplVulkan_InitInfo initInfo = {
        .ApiVersion          = VK_API_VERSION_1_4,
        .Instance            = Rhi::RenderContext::Instance()->GetVulkanInstance(),
        .PhysicalDevice      = Rhi::Device::Instance()->GetPhysicalDevice(),
        .Device              = Rhi::Device::Instance()->GetDevice(),
        .QueueFamily         = Rhi::Device::Instance()->GetQueueIndex( Rhi::QueueType_Graphics ),
        .Queue               = Rhi::Device::Instance()->GetQueue( Rhi::QueueType_Graphics ),
        .MinImageCount       = 3,
        .ImageCount          = 3,
        .MSAASamples         = VK_SAMPLE_COUNT_1_BIT,
        .PipelineCache       = Rhi::PipelineFactory::Instance()->GetPipelineCache(),
        .DescriptorPoolSize  = 10,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount    = 1,
            .pColorAttachmentFormats = &surfaceFormat,
            .depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT
        }
    };
    VkInstance instance = Rhi::RenderContext::Instance()->GetVulkanInstance();
    ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_4, [](const char *function_name, void *vulkan_instance) {
        return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance *>(vulkan_instance)), function_name);
      }, &instance);
    ImGui_ImplVulkan_Init( &initInfo );
    ImGui_ImplVulkan_CreateFontsTexture();
}

void GUI::Renderer::Destroy( void ) {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

void GUI::Renderer::Render( Rhi::CommandList * cmdlist ) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos( { 10.0f, 10.0f } );
    ImGui::SetNextWindowBgAlpha( 0.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
    ImGui::Begin( "Render Stats: ", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs );
    ImGui::Text( "FPS: %d", (int)Rhi::RenderStats::Instance()->fps );
    ImGui::Text( "Frametime: %.2f ms", 1000.0f / Rhi::RenderStats::Instance()->fps );
    ImGui::Text( "Render Resolution: %ux%u", Rhi::RenderStats::Instance()->renderResolution.x, Rhi::RenderStats::Instance()->renderResolution.y );
    ImGui::Text( "Draw Calls (CPU): %u", Rhi::RenderStats::Instance()->cpuDrawCalls );
    ImGui::Text( "Draw Calls (Indirect): %u", Rhi::RenderStats::Instance()->indirectDrawCalls );
    ImGui::Text( "Total VRAM used: %.2f GB", Rhi::RenderStats::Instance()->vRamUsedGB );
    ImGui::Text( "Total Vertices: %u", Rhi::RenderStats::Instance()->totalVertices );
    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::Render();

    cmdlist->BeginDebugLabel( "GUI", { 0.0f, 1.0f, 0.0f, 1.0f } );
    ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), cmdlist->mBuf );
    cmdlist->EndDebugLabel();
}
