#version 450

#include "structs.glsl"

layout(location=0) out vec4 out_color;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) buffer PointLights {
    PointLight point_lights[];
};

uniform uint index;

void main() {
    PointLight light = point_lights[index];
    out_color = vec4(0.0);
}