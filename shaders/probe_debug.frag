#version 450

layout(location = 0) in vec3 in_normal;

layout(location=0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

void main()
{
    out_color = vec4(1.0);
    out_normal = vec4(in_normal, 0.0);
}