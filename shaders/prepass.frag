#version 450

layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_color;

layout(binding = 0) uniform sampler2D in_texture;

uniform float alpha_cutoff;
uniform vec3 base_color_factor;

layout(location = 0) out vec4 out_color;

void main() {
    const vec4 albedo_tex = texture(in_texture, in_uv);
    const vec3 base_color = in_color.rgb * albedo_tex.rgb * base_color_factor;
    const float alpha = albedo_tex.a;

    if(alpha <= alpha_cutoff) {
        discard;
    }
    out_color = vec4(base_color, alpha);
}
