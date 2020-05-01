#version 450

layout (binding = 1) uniform sampler2DArray sampler_array;

layout (location = 0) in vec3 in_uv;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_color;

void main() {
	out_color = texture(sampler_array, in_uv) * in_color;
}