#ifndef PROBE_H
#define PROBE_H

#include <glm/vec3.hpp>
#include <vector>

#include "SceneObject.h"
#include "Texture.h"

namespace OM3D
{

    class Scene;
    class ProbeMap;

    class Probe
    {
    public:
        Probe(glm::vec3 position);
        void render(const Camera& c, glm::vec3 grid_index) const;

        void update_irradiance();

        friend ProbeMap;

    private:
        glm::vec3 _position;

        SceneObject _sphere_mesh;
    };

    class ProbeMap
    {
    public:
        using probe_data =
            std::vector<std::vector<std::vector<std::shared_ptr<Probe>>>>;

        ProbeMap(const Scene& s);
        void bake_all(const Scene& s);

        void render(const Camera& c) const;
        void bind(int index, const Scene& s) const;

        void bake_batch(const Scene& s);

        glm::uvec3 dim() const
        {
            return glm::uvec3(_dim);
        }
        u32 probe_count() const
        {
            return _probe_count;
        }

        glm::vec3 spacing() const { return _spacing; }
        glm::vec3 min_pos() const { return _probes[0][0][0]->_position; }

    private:
        probe_data _probes;
        glm::vec3 _dim;
        glm::vec3 _spacing;

        Texture _gbuffer_color_array;
        Texture _gbuffer_normal_array;
        Texture _gbuffer_depth_array;
        Texture _gbuffer_position_array;
        Texture _probe_radiance_array;

        Texture _capture_color;
        Texture _capture_normal;
        Texture _capture_depth;

        u32 _probe_count = 0;
        mutable u32 _next_probe_to_bake = 0;
    };

} // namespace OM3D

#endif // PROBE_H
