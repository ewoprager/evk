#pragma once

#include <vector>

//#define MSAA

#include <MoltenVK/mvk_vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <mattresses.h>

#include <SDL2/SDL_image.h>

#define MAX_FRAMES_IN_FLIGHT 2

#define VULKAN_VERSION_USING VK_API_VERSION_1_2

/*
 !!! BEWARE !!!
	 Be careful about the scopes of large blueprints that contain Vulkan 'create info's of various kinds.
	 If a Vulkan 'create info' has a pointer to some stuff, that pointer must remain valid at the point
	 of use of the outer-most blueprint!
 */

/*
 Initialisation:
	\/ For with SDL
	- `SDL_Init(...)`
	- `SDL_CreateWindow(..., SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | ...)`
	/\
	- Create an instance of `EVK::Devices`
		\/ For with SDL
		- Surface creation function:
`[window](VkInstance instance) -> VkSurfaceKHR {
	VkSurfaceKHR ret;
	if(SDL_Vulkan_CreateSurface(window, instance, &ret) == SDL_FALSE)
		throw std::runtime_error("failed to create window devices->surface!");
	return ret;
}`
		- Extent getter function:
 `[window]() -> VkExtent2D {
	 int width, height;
	 SDL_GL_GetDrawableSize(window, &width, &height);
	 return (VkExtent2D){
		 .width = uint32_t(width),
		 .height = uint32_t(height)
	 };
 }`
		/\
	- Create an instance of `EVK::Interface`
	- Add a window resize event callback to call method `EVK::Interface::FramebufferResizeCallback()` of the interface
		\/ For with SDL
		-   SDL_Event event;
			event.type = SDL_WINDOWEVENT;
			event.window.event = SDL_WINDOWEVENT_SIZE_CHANGED;
		/\
		\/ For with wxWidgets
		- Event type: `wxEVT_SIZE`
		/\
	- Fill required vertex and index buffers with `Vulkan::FillVertexBuffer` / `Vulkan::FillIndexBuffer`
	- Build structuers like UBOs, textures and buffered render passes
	- Build pipelines
 
 In the render loop:
	- (Access render pipelines with `EVK::Interface::RP(...)`, and pipeline descriptor sets with `EVK::Interface::RenderPipeline::DS(...)`)
 
	- Set the values of your uniforms by modifying the values pointed to by that returned/filled by `EVK::Interface::GetUniformBufferObjectPointer<<UBO struct type>>(...)` / `EVK::Interface::GetUniformBufferObjectPointers<<UBO struct type>>(...)`
	- Put your render pass in the following condition: `if(<EVK::Interface instance>.BeginFrame()){ ... }`
	- For each render pass:
		- Begin it with `EVK::Interface::CmdBegin(Layered)BufferedRenderPass(...)`, or `EVK::Interface::BeginFinalRenderPass()` for the final one
		- For each render pipeline:
			- Bind with `EVK::Interface::RenderPipeline::Bind()`
			Then, as many times as you want:
				- Set the desired descriptor set bindings with `EVK::Interface::RenderPipeline::BindDescriptorSets(...)` (this includes setting dynamic UBO offsets)
				- Set the desired push constant values with `EVK::Interface::RenderPipeline::CmdPushConstants<<PC struct type>>(...)`
				- Queue a draw call command `EVK::Interface::CmdDraw(...)` or `EVK::Interface::CmdDrawIndexed(...)`
		- Finish the pass with `EVK::Interface::CmdEndRenderPass()`, or `EVK::Interface::EndFinalRenderPassAndFrame()` for the final one
 
 Cleaning up:
	- `vkDeviceWaitIdle(EVK::Interface::GetLogicalDevice())`
	- Destroy your `EVK::Interface` instance
	- `SDL_DestroyWindow(<your SDL_Window pointer>)`
 */

namespace EVK {

template <typename T> T PositiveModulo(const T &lhs, const T &rhs){
	return ((lhs % rhs) + rhs) % rhs;
}

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsAndComputeFamily;
	std::optional<uint32_t> presentFamily;
	
	bool IsComplete(){
		return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
	}
};

} // namespace EVK
