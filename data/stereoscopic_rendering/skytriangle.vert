#version 410 core

uniform mat4 modelView;
uniform mat4 inverseProjection;

in vec2 in_vertex;

out vec2 v_uv;
out vec4 v_ray;

void main()
{
    gl_Position = vec4(in_vertex, 0.0, 1.0);

    v_uv = in_vertex.xy;
    

    v_ray =  modelView * vec4(in_vertex, 0.0, 1.0);
}
