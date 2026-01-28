struct CameraData {
    mat4 view_proj;
    mat4 inv_view_proj;
    vec3 position;
    float padding;
};

struct FrameData {
    CameraData camera;

    vec3 sun_dir;
    uint point_light_count;

    vec3 sun_color;
    float ibl_intensity;
    mat4 shadow_view_proj;

    float sun_bias;
    uint padding[3];

    // Set to vec4 because of padding
    vec4 probe_dim;
    vec4 probe_min_pos;
    vec4 probe_spacing;
    uint probe_count;
};

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float padding;
};

struct ProbeCamera {
    mat4 inv_view_proj[6];
};

const uint DebugAlbedoMode = 1;
const uint DebugNormalMode = 2;
const uint DebugRoughnessMode = 3;
const uint DebugMetallicMode = 4;
const uint DebugDepthMode = 5;
const uint DebugPositionMode = 6;
const uint DebugNoProbe = 7;