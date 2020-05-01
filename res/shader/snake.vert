#version 450

layout (binding = 0) uniform UBO {
	mat4 model;
	mat4 view;
	mat4 proj;
}ubo;

layout (location = 0) in vec3 in_vert_pos;		// binding = 0
layout (location = 1) in vec2 in_vert_uv;		// binding = 0
layout (location = 2) in vec3 in_inst_pos;		// binding = 1
layout (location = 3) in vec3 in_inst_scale;	// binding = 1
layout (location = 4) in vec4 in_inst_color;	// binding = 1
layout (location = 5) in uint in_inst_texid;	// binding = 1

layout (location = 0) out vec3 out_uv;			// vec3(u, v, texid)
layout (location = 1) out vec4 out_color;

out gl_PerVertex {
	vec4 gl_Position;
};

void main(){
	out_uv = vec3(in_vert_uv, in_inst_texid);
	out_color = in_inst_color;
	mat4 tran = mat4(0.0);
	tran[0].r = in_inst_scale.r;
	tran[1].g = in_inst_scale.g;
	tran[2].b = in_inst_scale.b;
	tran[3].a = 1.0;

	mat4 mvp = ubo.proj * ubo.view * ubo.model;

	gl_Position = mvp * ( vec4(in_inst_pos, 1.0) + tran * vec4(in_vert_pos, 1.0) );
	
	// gl_Position = in_inst_model * vec4(in_vert_pos, 1.0);

	// gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_pos + in_inst_pos, 1.0);
}