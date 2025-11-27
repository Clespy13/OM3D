#version 450

#include "utils.glsl"
#include "lighting.glsl"

// fragment shader of the main lighting pass

// #define DEBUG_NORMAL
// #define DEBUG_METAL
// #define DEBUG_ROUGH
// #define DEBUG_ENV

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_position;
layout(location = 4) in vec3 in_tangent;
layout(location = 5) in vec3 in_bitangent;

layout(binding = 0) uniform sampler2D in_texture;
layout(binding = 1) uniform sampler2D in_normal_texture;
layout(binding = 2) uniform sampler2D in_metal_rough;

uniform vec3 base_color_factor;
uniform vec2 metal_rough_factor;

layout(binding = 4) uniform samplerCube in_envmap;

layout(binding = 0) uniform Data {
    FrameData frame;
};

void main() {
    const vec3 normal_map = unpack_normal_map(texture(in_normal_texture, in_uv).xy);
    const vec3 normal = normal_map.x * in_tangent +
                        normal_map.y * in_bitangent +
                        normal_map.z * in_normal;

    const vec4 albedo_tex = texture(in_texture, in_uv);
    const vec3 base_color = in_color.rgb * albedo_tex.rgb * base_color_factor;

    const vec4 metal_rough_tex = texture(in_metal_rough, in_uv);
    const float roughness = metal_rough_tex.g * metal_rough_factor.y; // as per glTF spec
    const float metallic = metal_rough_tex.b * metal_rough_factor.x; // as per glTF spec

    out_color = vec4(base_color, roughness);
    out_normal = vec4(normal, metallic);
}