#version 450

#define PI 3.1415926

#include "structs.glsl"
#include "utils.glsl"

layout(location = 1) in vec2 in_uv;

layout(location=0) out vec4 out_color;

layout(binding = 0) uniform sampler2D in_albedo;
layout(binding = 1) uniform sampler2D in_normal;
layout(binding = 2) uniform sampler2D in_depth;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) buffer PointLights {
    PointLight point_lights[];
};

uniform uint index;

void main() {
    out_color = vec4(0.0);

    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const vec4 albedo = texelFetch(in_albedo, coord, 0);
    const vec4 normal_data = texelFetch(in_normal, coord, 0);
    const float depth = texelFetch(in_depth, coord, 0).r;

    const vec3 hdr = albedo.rgb;
    const vec3 normal = normal_data.rgb * 2.0 - 1.0;
    const float roughness = albedo.a;
    const float metallic = normal_data.a;
    const vec3 position = unproject(in_uv, depth, frame.camera.inv_view_proj);

    const vec3 to_view = (frame.camera.position - position);
    const vec3 view_dir = normalize(to_view);

    PointLight light = point_lights[index];

    float dist_sqr = dot(light.position, position);
    float intensity = 1.0 / (0.1 + dist_sqr);

    out_color = vec4(light.color * intensity, 0.0);
}
