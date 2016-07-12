#version 400 core

uniform vec2 scale;
uniform mat4 transform;

layout (location = 0) in vec4 in_vertex;

out vec4 v_color;

void main()
{
    gl_Position = transform * vec4(in_vertex.xyz, 1.0);

	v_color = vec4(in_vertex.xyz * 0.5 + 0.5, 
		clamp(in_vertex.y * 8.0, 0.0, 1.0));
}
