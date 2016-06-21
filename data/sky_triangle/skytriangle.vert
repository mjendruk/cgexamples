#version 420 core

in vec2 in_vertex;

out vec2 v_uv;

void main()
{
    gl_Position = vec4(in_vertex, 0.0, 1.0);
    v_uv = in_vertex.xy;
}
