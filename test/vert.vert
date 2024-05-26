#version 450

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec3 a_colour;

layout(location = 0) out vec3 v_colour;

void main() {
	v_colour = a_colour;
	gl_Position = vec4(a_position, 0.0, 0.5);
}
