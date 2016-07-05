#version 400 core

uniform mat4 transform;

//in vec2 in_vertex;
layout (location = 0) in vec4 in_vertex;

//out vec2 v_uv;
out vec4 v_color;


void main()
{
    //gl_Position = transform * vec4(in_vertex, 0.0, 1.0);
    gl_Position = transform * vec4(in_vertex.xyz, 1.0);
   // v_uv = in_vertex.xy;
   //v_color = mix(in_vertex * 0.5 + 0.5, vec3(.5), length(in_velocity));
   v_color = vec4(in_vertex.xyz * 0.5 + 0.5, smoothstep(0.0, 1.0, in_vertex.w * 0.5 + clamp(in_vertex.y * 8.0, 0.0, 1.0)));
}
