#version 150 core

uniform sampler2D source;
uniform vec2 advance;
uniform mat4 ndcInverse;

in vec2 v_uv;

out vec4 out_color;

const int kernelSize = 100;


void main()
{
	float sum = texture(source, v_uv).r;
	if(sum >= 1.0)
		discard;

	int kernelHalfSize = (kernelSize - 1) / 2;

	vec2 texelOffset = 1.0 / vec2(textureSize(source, 0));

	for(int i = -kernelHalfSize; i < 0; ++i)
		sum += texture(source, v_uv + i * advance * texelOffset).r;
	for(int i = 1; i <= kernelHalfSize; ++i)
		sum += texture(source, v_uv + i * advance * texelOffset).r;

	sum /= float(kernelSize);

	if(advance.x > 0.0)
	{
		//out_color = vec4(uv, sum.a, 0.0);
	}
	else
	{
		// retrieve normals

		//
	}

    
    vec2 uv = (v_uv - 0.5) * 2.0;
    vec4 pos = vec4(uv, sum, 1.0);

    pos = ndcInverse * pos;
    
    //out_color = vec4(uv, sum.a, 0.0);

    out_color = vec4(pos.xyz, 0.0);
    //out_color = vec4(sum.xyz, 0.0);
}
