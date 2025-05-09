#pragma once
#include <glm/glm.hpp>

namespace Entity {

    struct PointLight final {
        glm::vec3 position;  float _pad0;
        glm::vec3 color;
        float     intensity;
        float     linear;
        float     quadratic; float _pad2[2];
    };

    struct DirectionalLight final {
        glm::vec3 direction; float _pad0;
        glm::vec3 color;
    };

}
