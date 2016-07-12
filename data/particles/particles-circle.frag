#version 330 core

in vec2 g_uv;
in vec4 g_color;

out vec4 out_color;

void main()
{
	vec2 uv = g_uv;

	float v = dot(uv, uv);
	if(v > 1.0)
		discard;

	out_color = vec4(mix(vec3(1.0), g_color.xyz, g_color.w), 1.0);
}
