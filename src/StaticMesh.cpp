#include "StaticMesh.h"

#include <glad/gl.h>
#include <glm/gtx/norm.hpp>

namespace OM3D {

extern bool audit_bindings_before_draw;

namespace
{
    BoundingSphere compute_bounding_sphere(std::vector<Vertex> vertices)
    {
        BoundingSphere res;

        glm::vec3 vmin = vertices.front().position;
        glm::vec3 vmax = vertices.front().position;
        for (Vertex& v : vertices)
        {
            vmin.x = std::min(v.position.x, vmin.x);
            vmin.y = std::min(v.position.y, vmin.y);
            vmin.z = std::min(v.position.z, vmin.z);

            vmax.x = std::max(v.position.x, vmax.x);
            vmax.y = std::max(v.position.y, vmax.y);
            vmax.z = std::max(v.position.z, vmax.z);
        }

        float xdiff = vmax.x - vmin.x;
        float ydiff = vmax.y - vmin.y;
        float zdiff = vmax.z - vmin.z;

        float diameter = std::max(xdiff, std::max(ydiff, zdiff));

        res.radius = diameter * 0.5f;
        res.center = (vmax + vmin) * 0.5f;
        //res.center = vmin + (vmax - vmin) * 0.5f;

        for (Vertex& v : vertices)
        {
            glm::vec3 point = v.position;
            glm::vec3 direction = point - res.center;
            float sq_distance = glm::length2(direction);
            float sq_radius = res.radius * res.radius;

            if (sq_distance > sq_radius)
            {
                float distance = glm::sqrt(sq_distance);
                float new_radius = (res.radius + distance) * 0.5f;
                float offset = new_radius - res.radius;
                
                // Normalize direction and move center
                res.center += (offset / distance) * direction;
                res.radius = new_radius;
            }
        }

        return res;
    }
}

StaticMesh::StaticMesh(const MeshData& data) :
    _vertex_buffer(data.vertices),
    _index_buffer(data.indices),
    _bounding_sphere(compute_bounding_sphere(data.vertices)) {
}

void StaticMesh::draw() const {
    _vertex_buffer.bind(BufferUsage::Attribute);
    _index_buffer.bind(BufferUsage::Index);

    // Vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), nullptr);
    // Vertex normal
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    // Vertex uv
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
    // Tangent / bitangent sign
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(8 * sizeof(float)));
    // Vertex color
    glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(12 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    if(audit_bindings_before_draw) {
        audit_bindings();
    }

    glDrawElements(GL_TRIANGLES, int(_index_buffer.element_count()), GL_UNSIGNED_INT, nullptr);
}

const BoundingSphere& StaticMesh::bounding_sphere() const
{
    return _bounding_sphere;
}

}
