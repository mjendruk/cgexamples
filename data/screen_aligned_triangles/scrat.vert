// according GLSL version to OpenGL 3.2 core profile
#version 150 core

in vec2 in_vertex;

out vec2 v_uv;

void main()
{
    gl_Position = vec4(in_vertex * 2.0 + vec2(1.0, -1.0), 0.0, 1.0);
    v_uv = clamp(in_vertex.xy, vec2(-1.0), vec2(1.0));
}
