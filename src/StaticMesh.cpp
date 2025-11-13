#include "StaticMesh.h"

#include <glad/gl.h>

namespace OM3D {

extern bool audit_bindings_before_draw;

namespace
{
    BoundingSphere compute_bounding_sphere(std::vector<Vertex> vertices)
    {
        BoundingSphere res;

        glm::vec3 vmin = glm::vec3(0);
        glm::vec3 vmax = glm::vec3(0);
        for (Vertex& v : vertices)
        {
            if (v.position.x < vmin.x)
                vmin.x = v.position.x;
            if (v.position.y < vmin.y)
                vmin.y = v.position.y;
            if (v.position.z < vmin.z)
                vmin.z = v.position.z;

            if (v.position.x > vmax.x)
                vmax.x = v.position.x;
            if (v.position.y > vmax.y)
                vmax.y = v.position.y;
            if (v.position.z > vmax.z)
                vmax.z = v.position.z;
        }

        float xdiff = vmax.x - vmin.x;
        float ydiff = vmax.y - vmin.y;
        float zdiff = vmax.z - vmin.z;

        float diameter = std::max(xdiff, std::max(ydiff, zdiff));

        res.radius = diameter * 0.5f;
        res.center = vmin + (vmax - vmin) * 0.5f;

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
