#include "SceneObject.h"

#include <glm/gtc/matrix_transform.hpp>
#include "StaticMesh.h"

namespace OM3D {

SceneObject::SceneObject(std::shared_ptr<StaticMesh> mesh, std::shared_ptr<Material> material) :
    _mesh(std::move(mesh)),
    _material(std::move(material)) {
}

void SceneObject::render(Camera c) const {
    bool visible = is_visible(c);
    if(!_material || !_mesh || !visible) {
        return;
    }

    _material->set_uniform(HASH("model"), transform());
    _material->bind();
    _mesh->draw();
}

const Material& SceneObject::material() const {
    DEBUG_ASSERT(_material);
    return *_material;
}

void SceneObject::set_transform(const glm::mat4& tr) {
    _transform = tr;
}

const glm::mat4& SceneObject::transform() const {
    return _transform;
}

bool SceneObject::is_visible(Camera c) const {
    Frustum f = c.build_frustum();
    BoundingSphere s = _mesh->bounding_sphere();

    glm::vec3 sphere_world_pos = glm::vec3(transform() * glm::vec4(s.center, 1.0f));

    const glm::vec3 sx = glm::vec3(transform()[0]);
    const glm::vec3 sy = glm::vec3(transform()[1]);
    const glm::vec3 sz = glm::vec3(transform()[2]);
    const float scale_x = glm::length(sx);
    const float scale_y = glm::length(sy);
    const float scale_z = glm::length(sz);
    const float world_radius = s.radius * std::max(scale_x, std::max(scale_y, scale_z));

    glm::vec3 obj_dir = sphere_world_pos - c.position();

    const glm::vec3 near_n = glm::normalize(f._near_normal);
    const glm::vec3 left_n = glm::normalize(f._left_normal);
    const glm::vec3 right_n = glm::normalize(f._right_normal);
    const glm::vec3 top_n = glm::normalize(f._top_normal);
    const glm::vec3 bottom_n = glm::normalize(f._bottom_normal);

    float dot_near = glm::dot(near_n, obj_dir) + world_radius;
    float dot_left = glm::dot(left_n, obj_dir) + world_radius;
    float dot_right = glm::dot(right_n, obj_dir) + world_radius;
    float dot_top = glm::dot(top_n, obj_dir) + world_radius;
    float dot_bottom = glm::dot(bottom_n, obj_dir) + world_radius;

    return dot_near > 0 && dot_left > 0 && dot_right > 0 && dot_top > 0
        && dot_bottom > 0;
}

}
