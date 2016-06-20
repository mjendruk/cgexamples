#version 420 core

uniform sampler2D fragmentLocation;
uniform float threshold;
in vec2 v_uv;

out vec4 out_color;

void main()
{
	float index = texture(fragmentLocation, v_uv).r;
	if(threshold * 10 < index)
		discard;

	vec4 unpacked = unpackUnorm4x8(uint(index));
	out_color = vec4(unpacked.rgb * vec3(1.0, 1.0, 2.0), 1.0);
}
