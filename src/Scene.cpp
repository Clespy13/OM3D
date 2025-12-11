#include "Scene.h"

#include <TypedBuffer.h>

#include <glm/ext/matrix_transform.hpp>
#include <shader_structs.h>

#include <glad/gl.h>

#include <iostream>
#include <utility>
#include "ByteBuffer.h"
#include "glm/matrix.hpp"
#include "graphics.h"

namespace OM3D {

Scene::Scene() {
    _sky_material.set_program(Program::from_files("sky.frag", "screen.vert"));
    _sky_material.set_depth_test_mode(DepthTestMode::None);

    _depth_prepass_material.set_program(Program::from_files("prepass.frag", "basic.vert"));
    _depth_prepass_material.set_depth_test_mode(DepthTestMode::Standard);

    _shadow_pass_material.set_program(Program::from_files("prepass.frag", "basic.vert"));
    _shadow_pass_material.set_depth_test_mode(DepthTestMode::Standard);

    _envmap = std::make_shared<Texture>(Texture::empty_cubemap(4, ImageFormat::RGBA8_UNORM));
}

void Scene::add_object(SceneObject obj) {
    _objects.emplace_back(std::move(obj));
}

void Scene::add_light(PointLight obj) {
    _point_lights.emplace_back(std::move(obj));
}

Span<const SceneObject> Scene::objects() const {
    return _objects;
}

Span<const PointLight> Scene::point_lights() const {
    return _point_lights;
}

Camera& Scene::camera() {
    return _camera;
}

const Camera& Scene::camera() const {
    return _camera;
}

void Scene::set_envmap(std::shared_ptr<Texture> env) {
    _envmap = std::move(env);
}

void Scene::set_ibl_intensity(float intensity) {
    _ibl_intensity = intensity;
}

void Scene::set_sun(float altitude, float azimuth, glm::vec3 color, float bias) {
    // Convert from degrees to radians
    const float alt = glm::radians(altitude);
    const float azi = glm::radians(azimuth);
    // Convert from polar to cartesian
    _sun_direction = glm::vec3(sin(azi) * cos(alt), sin(alt), cos(azi) * cos(alt));
    _sun_color = color;
    _sun_bias = bias;
}

std::shared_ptr<Texture> Scene::depth_prepass_texture() const {
    return _depth_prepass_texture;
}

void Scene::depth_prepass() const {
    // Get current viewport size from OpenGL
    int viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    const glm::uvec2 viewport_size(viewport[2], viewport[3]);

    // Create depth prepass texture and framebuffer
    _depth_prepass_texture = std::make_shared<Texture>(viewport_size, ImageFormat::Depth32_FLOAT, WrapMode::Clamp);
    _depth_prepass_fbo = std::make_unique<Framebuffer>(_depth_prepass_texture.get());

    // Fill and bind frame data buffer
    TypedBuffer<shader::FrameData> buffer(nullptr, 1);
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        mapping[0].camera.view_proj = _camera.view_proj_matrix();
        mapping[0].camera.inv_view_proj = glm::inverse(_camera.view_proj_matrix());
        mapping[0].camera.position = _camera.position();
        mapping[0].point_light_count = u32(_point_lights.size());
        mapping[0].ibl_intensity = _ibl_intensity;
        mapping[0].sun_color = _sun_color;
        mapping[0].sun_dir = glm::normalize(_sun_direction);
    }
    buffer.bind(BufferUsage::Uniform, 0);

    // Render all opaque objects to depth buffer only
    _depth_prepass_fbo->bind(true, false);
    for(const SceneObject& obj : _objects) {
        obj.render(_camera);
    }
}

void Scene::shadow_pass() {
    const glm::uvec2 shadow_map_size(2048, 2048);

    // Create shadow pass texture and framebuffer
    glViewport(0, 0, shadow_map_size.x, shadow_map_size.y);
    _shadow_pass_texture = std::make_shared<Texture>(shadow_map_size, ImageFormat::Depth32_FLOAT, WrapMode::Clamp);
    glTextureParameteri(_shadow_pass_texture->handle(), GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(_shadow_pass_texture->handle(), GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
    glTextureParameteri(_shadow_pass_texture->handle(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(_shadow_pass_texture->handle(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    _shadow_pass_fbo = std::make_unique<Framebuffer>(_shadow_pass_texture.get());

    _shadow_cam = Camera();

    const glm::vec3 scene_center = glm::vec3(0.0f);
    const float scene_radius = 40.0f;

    const glm::vec3 sun_dir = glm::normalize(_sun_direction);
    const glm::vec3 shadow_cam_pos = scene_center - sun_dir * (scene_radius * 2.0f);

    const glm::vec3 up = glm::abs(sun_dir.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    _shadow_cam.set_view(glm::lookAt(-shadow_cam_pos, scene_center, up));

    const float extent = scene_radius * 1.2f;
    const float near_plane = 0.1f;
    const float far_plane = scene_radius * 4.0f;

    glm::mat4 shadow_proj = Camera::orthographic(-extent, extent, -extent, extent, near_plane, far_plane);
    _shadow_cam.set_proj(shadow_proj);

    // Fill and bind frame data buffer
    TypedBuffer<shader::FrameData> buffer(nullptr, 1);
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        mapping[0].camera.view_proj = _shadow_cam.view_proj_matrix();
        mapping[0].camera.inv_view_proj = glm::inverse(_shadow_cam.view_proj_matrix());
        mapping[0].camera.position = _shadow_cam.position();
        mapping[0].point_light_count = u32(_point_lights.size());
        mapping[0].ibl_intensity = _ibl_intensity;
        mapping[0].sun_color = _sun_color;
        mapping[0].sun_dir = glm::normalize(_sun_direction);
        mapping[0].sun_bias = _sun_bias;
        mapping[0].shadow_view_proj = _shadow_cam.view_proj_matrix();
    }
    buffer.bind(BufferUsage::Uniform, 0);

    // Render all opaque objects to depth buffer only
    _shadow_pass_fbo->bind(true, false);
    for(const SceneObject& obj : _objects) {
        obj.render_with_material(_shadow_cam, _shadow_pass_material);
    }
}

void Scene::render() const {
    // Get current viewport size
    int viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    const glm::uvec2 viewport_size(viewport[2], viewport[3]);

    // Save the current framebuffer
    GLint previous_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo);

    // Copy depth from prepass FBO to main FBO
    if(_depth_prepass_fbo) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previous_fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _depth_prepass_fbo->handle());

        glBlitFramebuffer(
            0, 0, viewport_size.x, viewport_size.y,
            0, 0, viewport_size.x, viewport_size.y,
            GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, previous_fbo);
    }

    if (_shadow_pass_fbo) {
        _shadow_pass_texture->bind(6);
    }

    // Fill and bind frame data buffer
    TypedBuffer<shader::FrameData> buffer(nullptr, 1);
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        mapping[0].camera.view_proj = _camera.view_proj_matrix();
        mapping[0].camera.inv_view_proj = glm::inverse(_camera.view_proj_matrix());
        mapping[0].camera.position = _camera.position();
        mapping[0].point_light_count = u32(_point_lights.size());
        mapping[0].sun_color = _sun_color;
        mapping[0].sun_dir = glm::normalize(_sun_direction);
        mapping[0].ibl_intensity = _ibl_intensity;
        mapping[0].sun_bias = _sun_bias;
        mapping[0].shadow_view_proj = _shadow_cam.view_proj_matrix();
    }
    buffer.bind(BufferUsage::Uniform, 0);

    // Fill and bind lights buffer
    TypedBuffer<shader::PointLight> light_buffer(nullptr, std::max(_point_lights.size(), size_t(1)));
    {
        auto mapping = light_buffer.map(AccessType::WriteOnly);
        for(size_t i = 0; i != _point_lights.size(); ++i) {
            const auto& light = _point_lights[i];
            mapping[i] = {
                light.position(),
                light.radius(),
                light.color(),
                0.0f
            };
        }
    }
    light_buffer.bind(BufferUsage::Storage, 1);

    // Bind envmap
    DEBUG_ASSERT(_envmap && !_envmap->is_null());
    _envmap->bind(4);

    // Bind brdf lut needed for lighting to scene rendering shaders
    brdf_lut().bind(5);

    // Render the sky
    _sky_material.bind();
    _sky_material.set_uniform(HASH("intensity"), _ibl_intensity);
    draw_full_screen_triangle();

    // Render every object
    {
        // Opaque first
        for(const SceneObject& obj : _objects) {
            if(obj.material().is_opaque()) {
                obj.render(_camera);
            }
        }

        // Transparent after
        for(const SceneObject& obj : _objects) {
            if(!obj.material().is_opaque()) {
                obj.render(_camera);
            }
        }
    }

}

std::pair<ByteBuffer, ByteBuffer> Scene::bind_light_pass_uniforms() const
{
    TypedBuffer<shader::FrameData> buffer(nullptr, 1);
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        mapping[0].camera.view_proj = _camera.view_proj_matrix();
        mapping[0].camera.inv_view_proj = glm::inverse(_camera.view_proj_matrix());
        mapping[0].camera.position = _camera.position();
        mapping[0].point_light_count = u32(_point_lights.size());
        mapping[0].ibl_intensity = _ibl_intensity;
        mapping[0].sun_color = _sun_color;
        mapping[0].sun_dir = glm::normalize(_sun_direction);
        mapping[0].sun_bias = _sun_bias;
        mapping[0].shadow_view_proj = _shadow_cam.view_proj_matrix();
    }
    buffer.bind(BufferUsage::Uniform, 0);

    TypedBuffer<shader::PointLight> light_buffer(nullptr, std::max(_point_lights.size(), size_t(1)));
    {
        auto mapping = light_buffer.map(AccessType::WriteOnly);
        for(size_t i = 0; i != _point_lights.size(); ++i) {
            const auto& light = _point_lights[i];
            mapping[i] = {
                light.position(),
                light.radius(),
                light.color(),
                0.0f
            };
        }
    }
    light_buffer.bind(BufferUsage::Storage, 1);

    _envmap->bind(4);
    brdf_lut().bind(5);
    _shadow_pass_texture->bind(6);

    return std::make_pair<ByteBuffer, ByteBuffer>(std::move(buffer), std::move(light_buffer));
}

}
