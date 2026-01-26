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

        void compute_gbuffer();
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

        void render(const Camera& c) const;
        void bind(int index) const;

        void bake_batch() const;

        glm::uvec3 dim() const
        {
            return glm::uvec3(_dim);
        }
        u32 probe_count() const
        {
            return _probe_count;
        }

    private:
        probe_data _probes;
        glm::vec3 _dim;

        Texture _gbuffer_color_array;
        Texture _gbuffer_normal_array;
        Texture _gbuffer_depth_array;

        Texture _capture_color;
        Texture _capture_normal;
        Texture _capture_depth;

        u32 _probe_count = 0;
        mutable u32 _next_probe_to_bake = 0;
    };

} // namespace OM3D

#endif // PROBE_H
