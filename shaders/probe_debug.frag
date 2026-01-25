#version 450

layout(location = 0) in vec3 in_normal;

layout(location=0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec3 out_id;

uniform vec3 grid_index;

void main()
{
    out_color = vec4(1.0);
    out_normal = vec4(in_normal, 0.0);
    out_id = grid_index;
}