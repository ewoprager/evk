#version 450

layout(set = 0, binding = 0) uniform UBO {
	float redOffset;
} ubo;

layout(location = 0) in vec3 v_colour;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 colour = v_colour;
	colour.r += ubo.redOffset;
	outColor = vec4(colour, 1.0);
}
