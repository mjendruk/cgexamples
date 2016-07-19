#version 330

uniform mat4 view;
uniform mat4 projection;
uniform mat4 ndcInverse;

uniform vec4 scale; // 1.0 / width, 1.0 / height, radius, aspect ratio

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

out vec2 g_uv;
out vec4 g_pos;
out float g_radius;

void main()
{
	vec4 p = gl_in[0].gl_Position;

	// frustum culling 
	vec4 pndc = projection * view * p;

	// this should actually test every generated position...
	vec2 c = clamp(abs(pndc.xy) / pndc.w, 0.0, 1.0);
	if(any(equal(c, vec2(1.0))))
		return;

	float radius = scale[2];

	g_pos = view * p;

	// create the front-most point
	vec4 fmp = view * (p + vec4(0.0, 0.0, radius, 0.0));
	// retrieve radius in view space
	g_radius = length(fmp - g_pos);

	vec4 u = vec4(1.0, 0.0, 0.0, 0.0);	
	vec4 v = vec4(0.0, scale[3], 0.0, 0.0);

	g_uv = vec2(-1.0, -1.0);
	gl_Position = projection * view * (p + radius * (ndcInverse * (-u -v)));

	EmitVertex();

	g_uv = vec2(-1.0,  1.0);
	gl_Position = projection * view * (p + radius * (ndcInverse * (-u +v)));
	EmitVertex();
	
	g_uv = vec2( 1.0, -1.0);
	gl_Position = projection * view * (p + radius * (ndcInverse * (+u -v)));
	EmitVertex();

	g_uv = vec2( 1.0,  1.0);
	gl_Position = projection * view * (p + radius * (ndcInverse * (+u +v)));
	EmitVertex();

	EndPrimitive();	
}