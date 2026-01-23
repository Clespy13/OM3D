#ifndef PROBE_H
#define PROBE_H

#include <vector>
#include <glm/vec3.hpp>

#include "SceneObject.h"
#include "Texture.h"

namespace OM3D {

class Scene;
class ProbeMap;

class Probe {
    public:
        Probe(glm::vec3 position);
        void render(const Camera &c) const;

        void compute_gbuffer();
        void update_irradiance();

        friend ProbeMap;

    private:
        glm::vec3 _position;

        SceneObject _sphere_mesh;
        Texture _gbuffer;
};

class ProbeMap {
    public:
        using probe_data = std::vector<std::vector<std::vector<std::shared_ptr<Probe>>>>;

        ProbeMap(const Scene& s);

        void render(const Camera &c) const;
        void bind(int index) const;

    private:
        probe_data _probes;
        glm::vec3 _dim;

        SceneObject _sphere_mesh;
        Texture _gbuffer;
};

}

#endif // PROBE_H