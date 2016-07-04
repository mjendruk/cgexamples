#version 330 core

in vec2 g_uv;
in vec3 g_color;

out vec4 out_color;

void main()
{
	vec2 uv = g_uv;
/*
	// point spread function
	float v = dot(uv, uv);
	if(v > 1.0)
		discard;

	v = smoothstep(1.0, 0.0, v);
	out_color = vec4(vec3(1.0), v * 0.1);
*/

	// sphere fake
	
	vec3 n = vec3(uv, dot(uv, uv));
	if(n.z > 1.0)
		discard;

	n.z = sqrt(1.0 - n.z);
	float l = dot(vec3(0.0, 0.0, 1.0), n);

	out_color = vec4(l * g_color.xyz, 1.0);
	//out_color = vec4(vec3(l), 0.3);
	
}
