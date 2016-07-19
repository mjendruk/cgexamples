#version 400 core

uniform vec2 scale;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec4 in_vertex;

//out vec4 v_color;

void main()
{
    gl_Position = projection * view * vec4(in_vertex.xyz, 1.0);
}
