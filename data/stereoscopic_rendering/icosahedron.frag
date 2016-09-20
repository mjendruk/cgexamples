#version 150 core

uniform vec3 color = vec3(0.51, 0.51, 0.60);

flat in vec3 v_normal;

out vec4 out_color;

const vec3 lightVec = (vec3(-2.5, 0.5, 1.5));

void main()
{
	float light1 = clamp(dot(lightVec, v_normal), 0, 1);
	float light2 = clamp(dot(v_normal, -lightVec), 0, 1);
	out_color = vec4(vec3(1.00, 0.87, 0.44) * light1 * 0.2 + vec3(0.27, 0.29, 0.80) * light2 * 0.2 + color * 0.6, 1.0);
}
