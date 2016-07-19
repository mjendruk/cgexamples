#version 330 core

in vec2 g_uv;

out vec4 out_color;

void main()
{
	vec2 uv = g_uv;

	float v = dot(uv, uv);
	if(v > 1.0)
		discard;

	vec3 n = vec3(uv, dot(uv, uv));
	n.z = -sqrt(1.0 - n.z);

	out_color = vec4(n * 0.5 + 0.5, 1.0);
}
