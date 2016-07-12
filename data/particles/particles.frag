#version 330 core

in vec4 v_color;

out vec4 out_color;

void main()
{
	out_color = vec4(mix(vec3(1.0), v_color.xyz, v_color.w), 1.0);
}
