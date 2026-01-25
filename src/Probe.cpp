#include "Probe.h"

#include <iostream>

#include "SceneObject.h"
#include "StaticMesh.h"

#include "Scene.h"
#include "TypedBuffer.h"
#include "graphics.h"
#include "shader_structs.h"

namespace OM3D
{

Probe::Probe(glm::vec3 position)
    : _position(position)
{
    static auto sphere_mesh = StaticMesh::from_gltf("../../data/sphere.glb");
    static auto mat_program = Program::from_files("probe_debug.frag", "basic.vert");

    if (!sphere_mesh.is_ok) {
        std::cerr << "Unable to load probe mesh.\n";
        return;
    }

    
    Material mat;
    mat.set_program(mat_program);

    _sphere_mesh = SceneObject(
        std::make_shared<StaticMesh>(sphere_mesh.value),
        std::make_shared<Material>(mat)
    );

    glm::mat4 transform = glm::mat4(1.0);
    transform = glm::translate(transform, position);
    transform = glm::scale(transform, glm::vec3(0.2f));
    _sphere_mesh.set_transform(transform);

    _gbuffer = Texture::empty_cubemap(256, ImageFormat::RGBA8_sRGB);
}

void Probe::render(const Camera &c, glm::vec3 grid_index) const
{
    _sphere_mesh.material().set_uniform(HASH("grid_index"), grid_index);
    _sphere_mesh.render(c);
}

void Probe::compute_gbuffer()
{
    // FIXME
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
    for (auto obj : s.objects()) {
        BoundingSphere s = obj.bounding_sphere();
        min_x = std::min(min_x, (s.center - s.radius).x);
        max_x = std::max(max_x, (s.center - s.radius).x);
        min_y = std::min(min_y, (s.center - s.radius).y);
        max_y = std::max(max_y, (s.center - s.radius).y);
        min_z = std::min(min_z, (s.center - s.radius).z);
        max_z = std::max(max_z, (s.center - s.radius).z);
    }

    float max_size = std::max(max_x - min_x, std::max(max_y - min_y, max_z - min_z));
    if (max_size == 0)
        return;

    int target_spacing = (max_size > 128) ? (int)(max_size / 128) + 2 : 3;

    int x_count = (int)((max_x - min_x) / target_spacing);
    int y_count = (int)((max_y - min_y) / target_spacing);
    int z_count = (int)((max_z - min_z) / target_spacing);

    if (x_count <= 0 || y_count <= 0 || z_count <= 0)
        return;

    _dim = glm::vec3(x_count, y_count, z_count);

    float x_spacing = (max_x - min_x) / (float)x_count;
    float y_spacing = (max_y - min_y) / (float)y_count;
    float z_spacing = (max_z - min_z) / (float)z_count;

    std::cout << "Rendering grid of " << x_count << "x" << y_count << "x"
              << z_count << " probes...\n";

    int counter = 0;
    int total = x_count * y_count * z_count;
    for (float z = 0; z < z_count; z++)
    {
        std::vector<std::vector<std::shared_ptr<Probe>>> level;
        for (float y = 0; y < y_count; y++)
        {
            std::vector<std::shared_ptr<Probe>> line;
            for (float x = 0; x < x_count; x++)
            {
                auto probe = std::make_shared<Probe>(
                    glm::vec3(x * x_spacing + min_x, y * y_spacing + min_y, z * z_spacing + min_z)
                );
                line.push_back(probe);

                std::cout << "\x1b[2K\r" << ++counter << " / " << total << std::flush;
            }
            level.push_back(line);
        }

        _probes.push_back(level);
    }

    std::cout << "\nRendering done.\n";
}

void ProbeMap::render(const Camera &c) const
{
    if (_dim == glm::vec3(0))
        return;

    for (int z = 0; z < _dim.z; z++) {
        for (int y = 0; y < _dim.y; y++) {
            for (int x = 0; x < _dim.x; x++) {
                _probes[z][y][x]->render(c, glm::vec3(
                    (float)x / 255.0f, (float)y / 255.0f, (float)z / 255.0f)
                );
            }
        }
    }
}

void ProbeMap::bind(int index) const
{
    int probe_count = (int)(_dim.x * _dim.y * _dim.z);
    TypedBuffer<shader::Probe> buffer(nullptr, std::max(probe_count, 1));

    int level_size = (int)(_dim.x * _dim.y);
    int line_size = (int)_dim.x;
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        for (int z = 0; z < _dim.z; z++) {
            for (int y = 0; y < _dim.y; y++) {
                for (int x = 0; x < _dim.x; x++) {
                    auto& probe = _probes[z][y][x];
                    mapping[z * level_size + y * line_size + x] = {
                        probe->_position
                    };
                }
            }
        }
    }
    buffer.bind(BufferUsage::Storage, index);
}

const Texture& ProbeMap::get_gbuffer(int x, int y, int z) const
{
    auto probe = _probes[z][y][x];
    return probe->_gbuffer;
}

}