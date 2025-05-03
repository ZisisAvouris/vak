#include <Renderer/RenderBase.hpp>

#include <vulkan/vulkan.h>

#define VOLK_IMPLEMENTATION
#include <volk.h>

#define VMA_IMPLEMENTATION
#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)
