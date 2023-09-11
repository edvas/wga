//
// Created by edvas on 9/10/23.
//

#ifndef WGA_SHADER_TYPES_HPP
#define WGA_SHADER_TYPES_HPP

#include <glm/glm.hpp>

namespace wga::shader_type
{
    struct uniforms {
        glm::mat4x4 projection_matrix;
        glm::mat4x4 view_matrix;
        glm::mat4x4 model_matrix;
        glm::vec4 color;
        float time{};
        [[maybe_unused]] float padding[3]{};
    };

    struct vertex_attributes{
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 uv;
    };
}

#endif //WGA_SHADER_TYPES_HPP
