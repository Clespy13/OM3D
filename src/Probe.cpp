#include "Probe.h"

#include <cmath>
#include <glad/gl.h>
#include <iostream>

#include "Camera.h"
#include "Framebuffer.h"
#include "ImageFormat.h"
#include "Scene.h"
#include "SceneObject.h"
#include "StaticMesh.h"
#include "Texture.h"
#include "TimestampQuery.h"
#include "TypedBuffer.h"
#include "graphics.h"
#include "shader_structs.h"

namespace OM3D
{

    Probe::Probe(glm::vec3 position)
        : _position(position)
    {
        static auto sphere_mesh =
            StaticMesh::from_gltf("../../data/sphere.glb");
        static auto mat_program =
            Program::from_files("probe_debug.frag", "basic.vert");

        if (!sphere_mesh.is_ok)
        {
            std::cerr << "Unable to load probe mesh.\n";
            return;
        }

        Material mat;
        mat.set_program(mat_program);

        _sphere_mesh =
            SceneObject(std::make_shared<StaticMesh>(sphere_mesh.value),
                        std::make_shared<Material>(mat));

        glm::mat4 transform = glm::mat4(1.0);
        transform = glm::translate(transform, position);
        transform = glm::scale(transform, glm::vec3(0.2f));
        _sphere_mesh.set_transform(transform);
    }

    void Probe::render(const Camera& c, glm::vec3 grid_index) const
    {
        _sphere_mesh.material().set_uniform(HASH("grid_index"), grid_index);
        _sphere_mesh.render(c);
    }

    void Probe::update_irradiance()
    {
        // FIXME
    }

    ProbeMap::ProbeMap(const Scene& s)
    {
        float min_x = std::numeric_limits<float>::infinity();
        float max_x = -std::numeric_limits<float>::infinity();
        float min_y = std::numeric_limits<float>::infinity();
        float max_y = -std::numeric_limits<float>::infinity();
        float min_z = std::numeric_limits<float>::infinity();
        float max_z = -std::numeric_limits<float>::infinity();
        for (auto obj : s.objects())
        {
            BoundingSphere bs = obj.bounding_sphere();
            const glm::vec3 min = bs.center - bs.radius;
            const glm::vec3 max = bs.center + bs.radius;
            min_x = std::min(min_x, min.x);
            max_x = std::max(max_x, max.x);
            min_y = std::min(min_y, min.y);
            max_y = std::max(max_y, max.y);
            min_z = std::min(min_z, min.z);
            max_z = std::max(max_z, max.z);
        }

        float max_size =
            std::max(max_x - min_x, std::max(max_y - min_y, max_z - min_z));
        if (max_size == 0)
            return;

        int target_spacing = (max_size > 128) ? (int)(max_size / 128) + 2 : 3;

        int x_count = (int)((max_x - min_x) / target_spacing);
        int y_count = (int)((max_y - min_y) / target_spacing);
        int z_count = (int)((max_z - min_z) / target_spacing);

        GLint max_array_layers = 0;
        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_array_layers);
        const int max_probes = std::max(1, max_array_layers / 6);
        int total = x_count * y_count * z_count;
        if (total > max_probes)
        {
            const float scale = cbrtf((float)total / (float)max_probes);
            target_spacing = int(target_spacing * scale) + 1;

            x_count = std::max(1, (int)((max_x - min_x) / target_spacing));
            y_count = std::max(1, (int)((max_y - min_y) / target_spacing));
            z_count = std::max(1, (int)((max_z - min_z) / target_spacing));

            total = x_count * y_count * z_count;
        }

        if (x_count <= 0 || y_count <= 0 || z_count <= 0)
            return;

        _dim = glm::vec3(x_count, y_count, z_count);

        float x_spacing = (max_x - min_x) / (float)x_count;
        float y_spacing = (max_y - min_y) / (float)y_count;
        float z_spacing = (max_z - min_z) / (float)z_count;
        _spacing = glm::vec3(x_spacing, y_spacing, z_spacing);

        std::cout << "Rendering grid of " << x_count << "x" << y_count << "x"
                  << z_count << " probes...\n";

        _probe_count = (u32)total;

        const u32 face_size = 128;
        _gbuffer_color_array = Texture::empty_cubemap_array(
            face_size, _probe_count, ImageFormat::RGBA8_UNORM);
        _gbuffer_normal_array = Texture::empty_cubemap_array(
            face_size, _probe_count, ImageFormat::RGBA8_UNORM);
        _gbuffer_depth_array = Texture::empty_cubemap_array(
            face_size, _probe_count, ImageFormat::Depth32_FLOAT);
        _gbuffer_position_array = Texture::empty_cubemap_array(
            face_size, _probe_count, ImageFormat::RGBA16_FLOAT);
        _probe_radiance_array = Texture::empty_cubemap_array(
            face_size, _probe_count, ImageFormat::RGBA16_FLOAT);

        _capture_color =
            Texture::empty_cubemap(face_size, ImageFormat::RGBA8_UNORM);
        _capture_normal =
            Texture::empty_cubemap(face_size, ImageFormat::RGBA8_UNORM);
        _capture_depth =
            Texture::empty_cubemap(face_size, ImageFormat::Depth32_FLOAT);

        int counter = 0;
        for (float z = 0; z < z_count; z++)
        {
            std::vector<std::vector<std::shared_ptr<Probe>>> level;
            for (float y = 0; y < y_count; y++)
            {
                std::vector<std::shared_ptr<Probe>> line;
                for (float x = 0; x < x_count; x++)
                {
                    auto probe = std::make_shared<Probe>(
                        glm::vec3(x * x_spacing + min_x, y * y_spacing + min_y,
                                  z * z_spacing + min_z));

                    line.push_back(probe);

                    std::cout << "\x1b[2K\r" << ++counter << " / " << total
                              << std::flush;
                }
                level.push_back(line);
            }

            _probes.push_back(level);
        }

        std::cout << "\nRendering done.\n";
    }

    void ProbeMap::render(const Camera& c) const
    {
        if (_dim == glm::vec3(0))
            return;

        for (int z = 0; z < _dim.z; z++)
        {
            for (int y = 0; y < _dim.y; y++)
            {
                for (int x = 0; x < _dim.x; x++)
                {
                    _probes[z][y][x]->render(c,
                                             glm::vec3((float)x / 255.0f,
                                                       (float)y / 255.0f,
                                                       (float)z / 255.0f));
                }
            }
        }
    }

    void ProbeMap::bind(int index) const
    {
        _probe_radiance_array.bind(index);
    }

    void bake_gbuffer_face(const Scene& s, glm::vec3 probe_pos,
                           std::array<Texture*, 4> buffers, int layer,
                           int face_index)
    {
        Framebuffer f = Framebuffer(
            buffers[0], std::array{ buffers[1], buffers[2], buffers[3] },
            layer);

        Material m;
        m.set_program(Program::from_files("probe_gbuffer.frag", "basic.vert"));

        Camera temp;
        temp.set_proj(Camera::perspective(to_rad(90), 1.0f, s.camera().near()));
        Camera cams[6] = {
            Camera(temp), Camera(temp), Camera(temp),
            Camera(temp), Camera(temp), Camera(temp),
        };

        cams[0].set_view(glm::lookAt(probe_pos, probe_pos + glm::vec3(1, 0, 0),
                                     glm::vec3(0, -1, 0)));
        cams[1].set_view(glm::lookAt(probe_pos, probe_pos + glm::vec3(-1, 0, 0),
                                     glm::vec3(0, -1, 0)));
        cams[2].set_view(glm::lookAt(probe_pos, probe_pos + glm::vec3(0, 1, 0),
                                     glm::vec3(0, 0, 1)));
        cams[3].set_view(glm::lookAt(probe_pos, probe_pos + glm::vec3(0, -1, 0),
                                     glm::vec3(0, 0, -1)));
        cams[4].set_view(glm::lookAt(probe_pos, probe_pos + glm::vec3(0, 0, 1),
                                     glm::vec3(0, -1, 0)));
        cams[5].set_view(glm::lookAt(probe_pos, probe_pos + glm::vec3(0, 0, -1),
                                     glm::vec3(0, -1, 0)));

        f.bind(true, true);
        s.render_cube(cams[face_index], m);
    }

    void ProbeMap::bake_batch(const Scene& s)
    {
        if (_probe_count == 0)
            return;

        constexpr u32 kBatchSize = 1;

        const u32 end =
            std::min(_probe_count, _next_probe_to_bake + kBatchSize);

        for (u32 i = _next_probe_to_bake; i < end; ++i)
        {
            const u32 base_layer = i * 6;
            const u32 level_size = (u32)(_dim.x * _dim.y);
            const u32 line_size = (u32)_dim.x;
            const u32 z = i / level_size;
            const u32 rem = i - z * level_size;
            const u32 y = rem / line_size;
            const u32 x = rem - y * line_size;
            const glm::vec3 probe_pos = _probes[z][y][x]->_position;
            for (u32 face = 0; face < 6; ++face)
            {
                const u32 layer = base_layer + face;
                bake_gbuffer_face(s, probe_pos,
                                  std::array{ &_gbuffer_depth_array,
                                              &_gbuffer_color_array,
                                              &_gbuffer_normal_array,
                                              &_gbuffer_position_array },
                                  layer, face);
            }

            auto buffers = s.bind_light_pass_uniforms();

            auto program = Program::from_file("probe_lighting.comp");
            program->bind();
            program->set_uniform(HASH("probe_position"), probe_pos);
            program->set_uniform(HASH("base_layer"), (int)base_layer);

            _gbuffer_color_array.bind_as_image(0, AccessType::ReadOnly);
            _gbuffer_normal_array.bind_as_image(1, AccessType::ReadOnly);
            _gbuffer_position_array.bind_as_image(2, AccessType::ReadOnly);
            _probe_radiance_array.bind_as_image(3, AccessType::WriteOnly);

            const auto size = _gbuffer_color_array.size();
            const u32 group_x = (size.x + 7) / 8;
            const u32 group_y = (size.y + 7) / 8;
            glDispatchCompute(group_x, group_y, 6);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        _next_probe_to_bake = (end == _probe_count) ? 0 : end;
    }

} // namespace OM3D
