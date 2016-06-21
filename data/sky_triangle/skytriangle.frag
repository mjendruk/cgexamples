#version 150 core

uniform samplerCube cubemap;

in vec2 v_uv;

out vec4 out_color;

void main()
{
	out_color = vec4(v_uv.xy * 0.5 + 0.5, 0.0, 1.0);
}
