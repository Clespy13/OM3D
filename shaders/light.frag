#version 450

#include "structs.glsl"
#include "utils.glsl"

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

layout(binding = 0) uniform sampler2D in_albedo;
layout(binding = 1) uniform sampler2D in_normal;

uniform uint debug_mode;

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const vec4 albedo = texelFetch(in_albedo, coord, 0);
    const vec4 normal_data = texelFetch(in_normal, coord, 0);

    const vec3 hdr = albedo.rgb;
    const vec3 normal = normal_data.rgb;
    const float roughness = albedo.a;
    const float metallic = normal_data.a;

    if (debug_mode == DebugAlbedoMode)
        out_color = vec4(hdr, 1.0);
    else if (debug_mode == DebugNormalMode)
        out_color = vec4(normal, 1.0);
    else if (debug_mode == DebugRoughnessMode)
        out_color = vec4(vec3(roughness), 1.0);
    else if (debug_mode == DebugMetallicMode)
        out_color = vec4(vec3(metallic), 1.0);
}


