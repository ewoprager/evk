#include "ShaderProgram.hpp"

static constexpr uint32_t PNGS_N = 3;

namespace Pipeline_MainInstanced {

namespace VertexShader {

struct Filename { static constexpr char string[] = "hello"; };
//constexpr char Filename::string[] = "Hello";
static_assert(EVK::filenameString_c<Filename>);

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

using type = EVK::VertexShader<Filename, PCS, Attributes,
EVK::UBOUniform<0, 0, false>
>;

static_assert(EVK::vertexShader_c<type>);

} // namespace VertexShader

namespace FragmentShader {

struct Filename { static constexpr char string[] = "hello"; };
//constexpr char Filename::string[] = "Hello";
static_assert(EVK::filenameString_c<Filename>);

struct PushConstantType {
	vec<4, float32_t> colourMult;
	vec<4, float32_t> specular;
	float32_t shininess;
	float32_t specularFactor;
	int32_t textureID;
};

using PCS = EVK::PushConstants<16, PushConstantType>;
static_assert(EVK::pushConstants_c<PCS>);

using type = EVK::Shader<VK_SHADER_STAGE_FRAGMENT_BIT, Filename, PCS,
EVK::UBOUniform<0, 0, false>,
EVK::TextureSamplersUniform<0, 1, 1>,
EVK::TextureImagesUniform<0, 2, PNGS_N>,
EVK::CombinedImageSamplersUniform<0, 3, 1>
>;
static_assert(EVK::shader_c<type>);

} // namespace FragmentShader

using type = EVK::RenderPipeline<VertexShader::type, FragmentShader::type>;

} // namespace Pipeline_MainInstanced

using Pipeline_MainInstancedT = Pipeline_MainInstanced::type;

int main(int argc, char *argv[]){
	
	std::cout << "Hello, World!\n";
	
	return 0;
}
