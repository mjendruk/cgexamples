#version 330 core

//#extension ARB_conservative_depth : require

uniform mat4 view;
uniform mat4 projection;
uniform mat3 normal;
uniform vec3 eye;

uniform vec4 scale; // 1.0 / width, 1.0 / height, radius, aspect ratio

in vec2 g_uv;
in vec4 g_pos;
in float g_radius;

//out float out_color;
out vec4 out_color;
//layout (depth_greater) out float gl_FragDepth;

const vec3 light = vec3(1.0, 1.0, 1.0);

void main()
{
	vec2 uv = g_uv;

	float v = dot(uv, uv);
	if(v > 1.0)
		discard;

	vec3 n = vec3(uv, dot(uv, uv));
	n.z = sqrt(1.0 - n.z);


	vec4 eye = g_pos + vec4(n * g_radius, 0.0);
	vec4 pos = projection * eye;
	pos.xyz /= pos.w;






	//n = normalize(normal * n);

	//float diffuse = max(0.0, dot(n, normalize(light)) + 1.0);
    //float RdotV = max(0.0, dot(normalize(reflect(light, n)), eye.xyz));
    //float specular = pow(RdotV, 1.0);

	//out_color = vec4(vec3(diffuse + specular) * 0.333, 1.0);
	
	out_color = vec4(n, 1.0);
	//out_color = vec4(eye.xyz, pos.z);
	//out_color = pos.z;

	gl_FragDepth = pos.z;
}
