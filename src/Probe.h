#ifndef PROBE_H
#define PROBE_H

#include <vector>
#include <memory>
#include <glm/vec3.hpp>

#include "SceneObject.h"

namespace OM3D {

class Scene;

class Probe {
    public:
        using probe_map = std::vector<std::shared_ptr<Probe>>;

        Probe(glm::vec3 position);
        void render(Camera c) const;

        void compute_gbuffer();
        void update_irradiance();

        static probe_map generate_probes(const Scene& s);

    private:
        glm::vec3 _position;

        SceneObject _sphere_mesh;
};

}

#endif // PROBE_H