#include <mattresses.h>

#include <ESDL/ESDL_General.h>
#include <ESDL/ESDL_EventHandler.h>

#include <SDL2/SDL_vulkan.h>

#include "ShaderProgram.hpp"
#include "Interface.hpp"

static constexpr uint32_t PNGS_N = 3;

namespace MainInstancedPipeline {

namespace VertexShader {

//struct Filename { static constexpr std::string string; };
//constexpr std::string Filename::string = "Hello";
//static_assert(EVK::filenameString_c<Filename>);

static constexpr char ch1[] = "somestrnig";

struct PushConstantType {
	int placeHolder;
};

using PCS = EVK::PushConstants<0, PushConstantType>;
static_assert(EVK::pushConstants_c<PCS>);

using Attributes = EVK::Attributes<EVK::TypePack<
vec<3>,
vec<3>,
vec<2>,
mat<4, 4>,
mat<4, 4>
>, EVK::BindingDescriptionPack<
VkVertexInputBindingDescription{
	0, // binding
	32,//sizeof(Vertex), // stride
	VK_VERTEX_INPUT_RATE_VERTEX // input rate
},
VkVertexInputBindingDescription{
	1, // binding
	128,//sizeof(Vertex), // stride
	VK_VERTEX_INPUT_RATE_INSTANCE // input rate
}
>, EVK::AttributeDescriptionPack<
VkVertexInputAttributeDescription{
	0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0//offsetof(Vertex, position)
},
VkVertexInputAttributeDescription{
	1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12//offsetof(Vertex, normal)
},
VkVertexInputAttributeDescription{
	2, 0, VK_FORMAT_R32G32_SFLOAT, 24//offsetof(Vertex, texCoord)
},
VkVertexInputAttributeDescription{
	3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0//offsetof(Vertex, position)
},
VkVertexInputAttributeDescription{
	4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16//offsetof(Vertex, position)
},
VkVertexInputAttributeDescription{
	5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32//offsetof(Vertex, position)
},
VkVertexInputAttributeDescription{
	6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 48//offsetof(Vertex, position)
},
VkVertexInputAttributeDescription{
	7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 64//offsetof(Vertex, normal)
},
VkVertexInputAttributeDescription{
	8, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 80//offsetof(Vertex, normal)
},
VkVertexInputAttributeDescription{
	9, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 96//offsetof(Vertex, normal)
},
VkVertexInputAttributeDescription{
	10, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 112//offsetof(Vertex, normal)
}
>>;

using type = EVK::VertexShader<ch1, PCS, Attributes,
EVK::UBOUniform<0, 0, false>
>;

static_assert(EVK::vertexShader_c<type>);

} // namespace VertexShader

namespace FragmentShader {

//struct Filename { static constexpr std::string string; };
//const std::string Filename::string = "Hello";
//static_assert(EVK::filenameString_c<Filename>);


static constexpr char ch1[] = "somestrnig";

struct PushConstantType {
	vec<4, float32_t> colourMult;
	vec<4, float32_t> specular;
	float32_t shininess;
	float32_t specularFactor;
	int32_t textureID;
};

using PCS = EVK::PushConstants<16, PushConstantType>;
static_assert(EVK::pushConstants_c<PCS>);

static_assert(EVK::pushConstantWithShaderStage_c<EVK::WithShaderStage<VK_SHADER_STAGE_FRAGMENT_BIT, PCS>>);

using type = EVK::Shader<VK_SHADER_STAGE_FRAGMENT_BIT, ch1, PCS,
EVK::UBOUniform<0, 0, false>,
EVK::TextureSamplersUniform<0, 1, 1>,
EVK::TextureImagesUniform<0, 2, PNGS_N>,
EVK::CombinedImageSamplersUniform<0, 3, 1>
>;
static_assert(EVK::shader_c<type>);

} // namespace FragmentShader

using type = EVK::RenderPipeline<VertexShader::type, FragmentShader::type>;

EVK::RenderPipelineBlueprint Blueprint(VkRenderPass renderPassHandle){
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
	
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.depthBiasEnable = VK_FALSE;
	colourBlending.attachmentCount = 1;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	dynamicState.dynamicStateCount = 2;
#ifdef MSAA
	multisampling.rasterizationSamples = interface->devices.GetMSAASamples();
#else
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
#endif
	
	return EVK::RenderPipelineBlueprint{
		.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.rasterisationStateCI = rasterizer,
		.multisampleStateCI = multisampling,
		.depthStencilStateCI = depthStencil,
		.colourBlendStateCI = colourBlending,
		.dynamicStateCI = dynamicState,
		.renderPassHandle = renderPassHandle
	};
}

} // namespace MainInstancedPipeline

using uniforms_t =
EVK::UniformsImpl<
EVK::TypePack<
	EVK::WithShaderStage<17, EVK::UBOUniform<0, 0>>,
	EVK::WithShaderStage<16, EVK::TextureSamplersUniform<0, 1>>,
	EVK::WithShaderStage<16, EVK::TextureImagesUniform<0, 2, 3>>,
	EVK::WithShaderStage<16, EVK::CombinedImageSamplersUniform<0, 3>>
>,
std::integer_sequence<unsigned int, 0>
>;
using pushConstantWithShaderStages_tp =
EVK::TypePack<
	EVK::WithShaderStage<1, EVK::PushConstants<0, MainInstancedPipeline::VertexShader::PushConstantType>>,
	EVK::WithShaderStage<16, EVK::PushConstants<16, MainInstancedPipeline::FragmentShader::PushConstantType>>
>;
static_assert(EVK::pushConstantWithShaderStage_c<EVK::WithShaderStage<1, EVK::PushConstants<0, MainInstancedPipeline::VertexShader::PushConstantType>>>);
static_assert(EVK::pushConstantWithShaderStage_c<EVK::WithShaderStage<16, EVK::PushConstants<16, MainInstancedPipeline::FragmentShader::PushConstantType>>>);

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
	
	std::shared_ptr<MainInstancedPipeline::type> mainInstancedPipeline = std::make_shared<MainInstancedPipeline::type>(devices, MainInstancedPipeline::Blueprint(interface->GetRenderPassHandle()));
	
	int time = SDL_GetTicks();
	
	while(!ESDL::HandleEvents()){
		const int newTime = SDL_GetTicks();
		const float dT = 0.001f*(float)(newTime - time);
		time = newTime;
		
		
		
	}
	
	SDL_DestroyWindow(window);
	
	return 0;
}
