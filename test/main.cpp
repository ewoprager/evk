#include <mattresses.h>

#include <ESDL/ESDL_General.h>
#include <ESDL/ESDL_EventHandler.h>

#include <SDL2/SDL_vulkan.h>

#include "ShaderProgram.hpp"
#include "Interface.hpp"

struct Vertex {
	vec<2, float32_t> position;
	vec<3, float32_t> colour;
};

namespace Pipeline {

namespace VertexShader {

static constexpr char vertexFilename[] = "../Shaders/vert.spv";

struct PushConstantType {
	mat<4, 4> projViewModel;
};
using PCS = EVK::PushConstants<0, PushConstantType>;
static_assert(EVK::pushConstants_c<PCS>);

using Attributes = EVK::Attributes<EVK::BindingDescriptionPack<
VkVertexInputBindingDescription{
	0, // binding
	20, // stride
	VK_VERTEX_INPUT_RATE_VERTEX // input rate
}
>, EVK::AttributeDescriptionPack<
VkVertexInputAttributeDescription{
	0, 0, VK_FORMAT_R32G32_SFLOAT, 0
},
VkVertexInputAttributeDescription{
	1, 0, VK_FORMAT_R32G32B32_SFLOAT, 8
}
>>;

using type = EVK::VertexShader<vertexFilename, PCS, Attributes
//, EVK::UBOUniform<0, 0, false>
>;

static_assert(EVK::vertexShader_c<type>);

} // namespace VertexShader

namespace FragmentShader {

static constexpr char fragmentFilename[] = "../Shaders/frag.spv";

//struct PushConstantType {
//	vec<4, float32_t> colourMult;
//	vec<4, float32_t> specular;
//	float32_t shininess;
//	float32_t specularFactor;
//	int32_t textureID;
//};

//using PCS = EVK::PushConstants<16, PushConstantType>;
//static_assert(EVK::pushConstants_c<PCS>);

//static_assert(EVK::pushConstantWithShaderStage_c<EVK::WithShaderStage<VK_SHADER_STAGE_FRAGMENT_BIT, PCS>>);

struct UBO {
	float redOffset;
};

using type = EVK::Shader<VK_SHADER_STAGE_FRAGMENT_BIT, fragmentFilename, EVK::NoPushConstants
, EVK::UBOUniform<0, 0, UBO>
//,
//EVK::TextureSamplersUniform<0, 1, 1>,
//EVK::TextureImagesUniform<0, 2, PNGS_N>,
//EVK::CombinedImageSamplersUniform<0, 3, 1>
>;
static_assert(EVK::shader_c<type>);

} // namespace FragmentShader

using type = EVK::RenderPipeline<VertexShader::type, FragmentShader::type>;

std::shared_ptr<type> Build(std::shared_ptr<EVK::Devices> devices, VkRenderPass renderPassHandle){
	
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	
	//If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them. This is useful in some special cases like shadow maps.
	 
	//Using this requires enabling a GPU feature.
	
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	
	//The polygonMode determines how fragments are generated for geometry. The following modes are available:
	 
	//VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
	//VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
	//VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
	 
	//Using any mode other than fill requires enabling a GPU feature.
	
	rasterizer.lineWidth = 1.0f;
	//The lineWidth member is straightforward, it describes the thickness of lines in terms of number of fragments. The maximum line width that is supported depends on the hardware and any line thicker than 1.0f requires you to enable the wideLines GPU feature.
	
	//rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	//The cullMode variable determines the type of face culling to use. You can disable culling, cull the front faces, cull the back faces or both. The frontFace variable specifies the vertex order for faces to be considered front-facing and can be clockwise or counterclockwise.
	
	//rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
	
	
	// ----- Multisampling behaviour -----
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	//#ifdef MSAA
	//multisampling.rasterizationSamples = device.GetMSAASamples();
	//#else
	//multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	//#endif
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional
	// This is how to achieve anti-aliasing.
	// Enabling it requires enabling a GPU feature
	
	
	// ----- Colour blending behaviour -----
	VkPipelineColorBlendStateCreateInfo colourBlending{};
	VkPipelineColorBlendAttachmentState colourBlendAttachment{};
	colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colourBlendAttachment.blendEnable = VK_TRUE;
	colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	
	colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlending.logicOpEnable = VK_FALSE;
	colourBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	//colourBlending.attachmentCount = 1;
	colourBlending.pAttachments = &colourBlendAttachment;
	colourBlending.blendConstants[0] = 0.0f; // Optional
	colourBlending.blendConstants[1] = 0.0f; // Optional
	colourBlending.blendConstants[2] = 0.0f; // Optional
	colourBlending.blendConstants[3] = 0.0f; // Optional
	
	
	// ----- Depth stencil behaviour -----
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	//depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional
	
	// ----- Dynamic state -----
	VkPipelineDynamicStateCreateInfo dynamicState{};
	VkDynamicState dynamicStates[3] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS
	};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	//dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;
	
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.depthBiasEnable = VK_FALSE;
#ifdef MSAA
	multisampling.rasterizationSamples = devices.GetMSAASamples();
#else
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
#endif
	colourBlending.attachmentCount = 1;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	dynamicState.dynamicStateCount = 2;
	
	const EVK::RenderPipelineBlueprint blueprint = {
		.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.pRasterisationStateCI = &rasterizer,
		.pMultisampleStateCI = &multisampling,
		.pDepthStencilStateCI = &depthStencil,
		.pColourBlendStateCI = &colourBlending,
		.pDynamicStateCI = &dynamicState,
		.renderPassHandle = renderPassHandle
	};
	
	return std::make_shared<type>(devices, &blueprint);
}

} // namespace Pipeline

const Vertex vertices[3] = {
	{{ 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
	{{ 1.0f,-1.0f}, {0.0f, 1.0f, 0.0f}},
	{{-1.0f,-1.0f}, {0.0f, 0.0f, 1.0f}}
};

int main(int argc, char *argv[]){
	std::cout << "Hello, World!\n";
	
	SDL_Init(SDL_INIT_EVERYTHING);
	
	ESDL::InitEventHandler();
	
	SDL_Window *window = SDL_CreateWindow("EVK Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	
	uint32_t requiredExtensionCount;
	SDL_Vulkan_GetInstanceExtensions(window, &requiredExtensionCount, nullptr);
	std::vector<const char *> requiredExtensions(requiredExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &requiredExtensionCount, requiredExtensions.data());
	std::shared_ptr<EVK::Devices> devices = std::make_shared<EVK::Devices>("EVK Test", requiredExtensions,
																		   [window](VkInstance instance) -> VkSurfaceKHR {
		VkSurfaceKHR ret;
		if(SDL_Vulkan_CreateSurface(window, instance, &ret) == SDL_FALSE)
			throw std::runtime_error("failed to create window devices.surface!");
		return ret;
	},
																		   [window]() -> VkExtent2D {
		int width, height;
		SDL_GL_GetDrawableSize(window, &width, &height);
		return (VkExtent2D){
			.width = uint32_t(width),
			.height = uint32_t(height)
		};
	});
	
	std::shared_ptr<EVK::Interface> interface = std::make_shared<EVK::Interface>(devices);
	
	std::shared_ptr<Pipeline::type> mainInstancedPipeline = Pipeline::Build(devices, interface->GetRenderPassHandle());
	
	std::shared_ptr<EVK::VertexBufferObject> vbo = std::make_shared<EVK::VertexBufferObject>(devices);
	
	vbo->Fill((void *)(vertices), sizeof(vertices), 0);
	
	const std::function<mat<4, 4>()> projFunction = [&]() -> mat<4, 4> {
		return mat<4, 4>::PerspectiveProjection(M_PI*0.25f, (float)interface->GetExtentWidth() / (float)interface->GetExtentHeight(), 0.1f, 10.0f);
	};
	
	const std::function<mat<4, 4>(const vec<3> &, const vec<3> &)> viewFunction = [&](const vec<3> &cameraPosition, const vec<3> &cameraRPY) -> mat<4, 4> {
		const mat<4, 4> camera = mat<4, 4>::Translation(cameraPosition) & mat<4, 4>::YRotation(cameraRPY.z) & mat<4, 4>::XRotation(cameraRPY.y) & mat<4, 4>::ZRotation(cameraRPY.x);
		return camera.Inverted();
	};
	
	const float spinSpeed = 0.4f;
	
	const std::function<mat<4, 4>(float)> modelFunction = [&](float timeS) -> mat<4, 4> {
		return mat<4, 4>::YRotation(spinSpeed * timeS);
	};
	
	const std::function<Pipeline::VertexShader::PushConstantType(float)> pcsFunction = [&](float timeS) -> Pipeline::VertexShader::PushConstantType {
		return {
			projFunction() & viewFunction(vec<3>{0.0f, 0.0f, 5.0f}, vec<3>{0.0f, -0.01f, 0.0f}) & modelFunction(timeS)
		};
	};
	
	std::shared_ptr<EVK::UniformBufferObject<Pipeline::FragmentShader::UBO>> ubo = std::make_shared<EVK::UniformBufferObject<Pipeline::FragmentShader::UBO>>(devices);
	
	mainInstancedPipeline->iDescriptorSet<0>().template iDescriptor<0>().Set(ubo);
	
	const std::function<void(float, uint32_t)> updateFunction = [&](float timeS, uint32_t flight) {
		ubo->GetDataPointer(flight)->redOffset = 2.0f * sin(timeS);
	};
	
	int time = SDL_GetTicks();
	
	while(!ESDL::HandleEvents()){
		const int newTime = SDL_GetTicks();
//		const float dT = 0.001f*(float)(newTime - time);
		time = newTime;
		
		const float timeS = 0.001f * static_cast<float>(time);
		
		if(std::optional<EVK::Interface::FrameInfo> fi = interface->BeginFrame(); fi.has_value()){
			updateFunction(timeS, fi->frame);
			
			interface->BeginFinalRenderPass({{0.01f, 0.01f, 0.01f, 1.0f}});
			
			mainInstancedPipeline->CmdBind(fi->cb);
			if(mainInstancedPipeline->CmdBindDescriptorSets<0, 1>(fi->cb, fi->frame) && vbo->CmdBind(fi->cb, 0)){
				Pipeline::VertexShader::PushConstantType pcs = pcsFunction(timeS);
				mainInstancedPipeline->CmdPushConstants<0>(fi->cb, &pcs);
				interface->CmdDraw(3);
			} else {
				std::cout << "Failing to draw.\n";
			}
			
			interface->EndFinalRenderPassAndFrame();
		} else {
			std::cout << "Failed to begin frame.\n";
		}
		
	}
	
	std::cout << "Exiting.\n";
	
	SDL_DestroyWindow(window);
	
	return 0;
}
