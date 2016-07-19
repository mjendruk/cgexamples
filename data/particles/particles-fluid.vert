#version 400 core

layout (location = 0) in vec4 in_vertex;

void main()
{
    gl_Position = vec4(in_vertex.xyz, 1.0);
}
