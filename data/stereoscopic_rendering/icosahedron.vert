#version 410 core

uniform mat4 modelView;
uniform mat4 projection;

in vec3 in_vertex;
in vec3 in_normal;

flat out vec3 v_normal;

void main()
{
    gl_Position = projection * modelView * vec4(in_vertex, 1.0);
    v_normal = in_normal;
}
