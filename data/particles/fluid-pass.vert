#version 410 core

layout (location = 0) in vec4 in_vertex;

out vec2 v_uv;

void main()
{
    gl_Position = vec4(in_vertex.xy, 0.0, 1.0);
    v_uv = in_vertex.xy * 0.5 + 0.5;
}
