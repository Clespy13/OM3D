#version 450

#include "structs.glsl"
#include "utils.glsl"
#include "lighting.glsl"

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

layout(binding = 0) uniform sampler2D in_albedo;
layout(binding = 1) uniform sampler2D in_normal;
layout(binding = 2) uniform sampler2D in_depth;
// layout(binding = 3) uniform sampler2D in_emissive;

layout(binding = 4) uniform samplerCube in_envmap;
layout(binding = 5) uniform sampler2D brdf_lut;
layout(binding = 6) uniform sampler2DShadow in_shadow;
layout(binding = 7) uniform samplerCubeArray probe_radiance;

uniform uint debug_mode;

layout(binding = 0) uniform Data {
    FrameData frame;
};

vec3 get_probe_color(vec3 probe_pos, vec3 position) {
    float layer = probe_pos.x + probe_pos.y * frame.probe_dim.x + probe_pos.z * frame.probe_dim.x * frame.probe_dim.y;

    vec3 probe_cube_center = frame.probe_min_pos + probe_pos * frame.probe_spacing;
    vec3 dir = normalize(position - probe_cube_center);
    return texture(probe_radiance, vec4(dir, layer)).rgb;
}

vec3 probe_lighting(vec3 position) {
    vec3 start = position - frame.probe_min_pos;
    vec3 corner_pos = start / frame.probe_spacing;

    if (frame.probe_dim.x == 0 || frame.probe_dim.y == 0 || frame.probe_dim.z == 0)
        return vec3(0.0);

    vec3 p0 = floor(corner_pos);
    vec3 p1 = p0 + 1.0;
    p0 = clamp(p0, vec3(0.0), frame.probe_dim - 1.0);
    p1 = clamp(p1, vec3(0.0), frame.probe_dim - 1.0);

    // NOTE: Si je me suis pas trompée, le 000 c'est le coin avant en bas à gauche
    vec3 c000 = get_probe_color(vec3(p0.x, p0.y, p0.z), position);
    vec3 c100 = get_probe_color(vec3(p1.x, p0.y, p0.z), position);
    vec3 c010 = get_probe_color(vec3(p0.x, p1.y, p0.z), position);
    vec3 c110 = get_probe_color(vec3(p1.x, p1.y, p0.z), position);
    vec3 c001 = get_probe_color(vec3(p0.x, p0.y, p1.z), position);
    vec3 c101 = get_probe_color(vec3(p1.x, p0.y, p1.z), position);
    vec3 c011 = get_probe_color(vec3(p0.x, p1.y, p1.z), position);
    vec3 c111 = get_probe_color(vec3(p1.x, p1.y, p1.z), position);

    vec3 off = corner_pos - p0;

    vec3 c00 = mix(c000, c100, off.x);
    vec3 c01 = mix(c001, c101, off.x);
    vec3 c10 = mix(c010, c110, off.x);
    vec3 c11 = mix(c011, c111, off.x);

    vec3 c0 = mix(c00, c10, off.y);
    vec3 c1 = mix(c01, c11, off.y);

    vec3 result = mix(c0, c1, off.z);
    return c00;
}

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const vec4 albedo = texelFetch(in_albedo, coord, 0);
    const vec4 normal_data = texelFetch(in_normal, coord, 0);
    const float depth = texelFetch(in_depth, coord, 0).r;

    const vec3 hdr = albedo.rgb;
    const vec3 normal = normal_data.rgb * 2.0 - 1.0;
    const float roughness = albedo.a;
    const float metallic = normal_data.a;
    const vec3 position = unproject(in_uv, depth, frame.camera.inv_view_proj);

    if (debug_mode == DebugAlbedoMode)
        out_color = vec4(hdr, 1.0);
    else if (debug_mode == DebugNormalMode)
        out_color = vec4(normal, 1.0);
    else if (debug_mode == DebugRoughnessMode)
        out_color = vec4(vec3(roughness), 1.0);
    else if (debug_mode == DebugMetallicMode)
        out_color = vec4(vec3(metallic), 1.0);
    else if (debug_mode == DebugDepthMode)
        out_color = vec4(vec3(pow(depth, 0.35)), 1.0);
    else if (debug_mode == DebugPositionMode)
        out_color = vec4(position, 1.0);
    else
    {
        const vec3 to_view = (frame.camera.position - position);
        const vec3 view_dir = normalize(to_view);

        float shadow = 0.0;
        {
            // Transform position to shadow camera clip space
            vec4 shadow_clip = frame.shadow_view_proj * vec4(position, 1.0f);
            vec3 shadow_ndc = shadow_clip.xyz / shadow_clip.w;

            // [-1,1] to [0,1]
            vec3 shadow_uv = vec3(shadow_ndc.xy * 0.5 + 0.5, shadow_ndc.z);

            if (shadow_uv.x >= 0.0 && shadow_uv.x <= 1.0 && 
                shadow_uv.y >= 0.0 && shadow_uv.y <= 1.0 &&
                shadow_uv.z >= 0.0 && shadow_uv.z <= 1.0) {
                shadow_uv.z += frame.sun_bias;

                for (float i = -1; i <= 1; i++) {
                    for (float j = -1; j <= 1; j++) {
                        vec2 UV_offseted = shadow_uv.xy + (vec2(i, j) / textureSize(in_shadow, 0).xy); // Offset the UV
                        shadow += texture(in_shadow, vec3(UV_offseted, shadow_uv.z));  // Accumulate the samples
                    }
                }
                shadow /= 9; // divide by the number of the samples
            } else {
                // clamp to far plane
                shadow = 1.0;
            }
        }

        // NOTE: on a pas besoin de gérer l'emissive
        // vec3 acc = texture(in_emissive, in_uv).rgb * emissive_factor;
        vec3 acc = eval_ibl(in_envmap, brdf_lut, normal, view_dir, albedo.rgb, metallic, roughness) * frame.ibl_intensity;
        acc += frame.sun_color * eval_brdf(normal, view_dir, frame.sun_dir, albedo.rgb, metallic, roughness) * shadow;

        vec3 lighting = probe_lighting(position);
        acc += lighting;

        out_color = vec4(acc, 1.0);
    }
}


