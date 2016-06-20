#version 420 core

uniform sampler2D fragmentLocation;
uniform float threshold;
in vec2 v_uv;

out vec4 out_color;

vec3 hsl2rgb(in vec3 hsl)
{
	// https://www.shadertoy.com/view/4sS3Dc

	vec3 rgb = clamp( abs(mod(hsl[0] + vec3(0.0, 4.0, 2.0), 6.0) - 3.0)- 1.0, 0.0, 1.0);
	return hsl[2] + hsl[1] * (rgb - 0.5) * (1.0 - abs(2.0 * hsl[2] - 1.0));
}

void main()
{
	float index = texture(fragmentLocation, v_uv).r;
	if(threshold < index)
		discard;

	vec4 unpacked = unpackUnorm4x8(uint(index));
	vec3 color = hsl2rgb(vec3(unpacked.b * 20.0
		, unpacked.r * 1.0, unpacked.g * 0.75 + 0.125));

	float f = clamp(smoothstep(0.0, 1.0, float(threshold - index) * 0.00001), 0.0, 1.0);
	
	out_color = vec4(mix(vec3(1.0), color, f), 1.0);
}
