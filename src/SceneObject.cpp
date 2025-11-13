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

    glm::vec3 sphere_world_pos = transform() * glm::vec4(s.center, 0.0);
    glm::vec3 obj_dir = sphere_world_pos - c.position();

    float dot_near = glm::dot(f._near_normal, obj_dir + f._near_normal * s.radius);
    float dot_left = glm::dot(f._left_normal, obj_dir + f._left_normal * s.radius);
    float dot_right = glm::dot(f._right_normal, obj_dir + f._right_normal * s.radius);
    float dot_top = glm::dot(f._top_normal, obj_dir + f._top_normal * s.radius);
    float dot_bottom = glm::dot(f._bottom_normal, obj_dir + f._bottom_normal * s.radius);

    return dot_near > 0 && dot_left > 0 && dot_right > 0 && dot_top > 0
        && dot_bottom > 0;
}

}
