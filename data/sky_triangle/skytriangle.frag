#version 150 core

in vec3 v_normal;

out vec4 out_color;

const vec3 lightVec = vec3(-2.0, 0.0, 2.0);

void main()
{
	float light = dot(lightVec, v_normal);
	out_color = vec4(((v_normal * 0.5) + 0.5) * 0.7 + vec3(light * 0.1), 1.0);
}
