#version 330

uniform mat4 view;
uniform mat4 projection;
uniform mat4 ndcInverse;

uniform vec4 scale; // 1.0 / width, 1.0 / height, radius, aspect ratio

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in vec4 v_color[];

out vec2 g_uv;
out vec4 g_color;

void main()
{
	vec4 p = gl_in[0].gl_Position;

	vec2 u = vec2(1.0, 0.0);	
	vec2 v = vec2(0.0, scale[3]);

	// frustum culling 
	vec4 pndc = projection * view * p;

	vec2 c = clamp(abs(pndc.xy) / pndc.w, 0.0, 1.0);
	if(any(equal(c, vec2(1.0))))
		return;

	g_color = v_color[0];

	//p /= p.w; // uncomment this to provide fair 
		// comparison between Point and Quad drawing ... (same fillrate)

	float radius = scale[2];

	g_uv = vec2(-1.0, -1.0);
	gl_Position = projection * view * (p + radius * (ndcInverse * vec4(-u -v, 0.0, 0.0)));
	EmitVertex();

	g_uv = vec2(-1.0,  1.0);
	gl_Position = projection * view * (p + radius * (ndcInverse * vec4(-u +v, 0.0, 0.0)));
	EmitVertex();
	
	g_uv = vec2( 1.0, -1.0);
	gl_Position = projection * view * (p + radius * (ndcInverse * vec4(+u -v, 0.0, 0.0)));
	EmitVertex();

	g_uv = vec2( 1.0,  1.0);
	gl_Position = projection * view * (p + radius * (ndcInverse * vec4(+u +v, 0.0, 0.0)));
	EmitVertex();

	EndPrimitive();	
}