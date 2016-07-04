#version 400 core

uniform mat4 transform;

//in vec2 in_vertex;
layout (location = 0) in vec3 in_vertex;
layout (location = 1) in vec3 in_velocity;

//out vec2 v_uv;
out float v_scale;
out vec3 v_color;


void main()
{
    //gl_Position = transform * vec4(in_vertex, 0.0, 1.0);
    v_scale = 0.02;

    gl_Position = transform * vec4(in_vertex, 1.0);
   // v_uv = in_vertex.xy;
   //v_color = mix(in_vertex * 0.5 + 0.5, vec3(.5), length(in_velocity));
   v_color = (in_vertex * 0.5 + 0.5);
}
