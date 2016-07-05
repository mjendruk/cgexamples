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

	vec3 n = vec3(uv, dot(uv, uv));
	n.z = sqrt(1.0 - n.z) * 0.5 + 0.5;

	out_color = vec4(mix(vec3(1.0), n.z * g_color.xyz, g_color.w), 1.0);
}
