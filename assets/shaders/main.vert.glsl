#version 460 core
#extension GL_EXT_buffer_reference : require

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec3 normal;
layout ( location = 2 ) in vec2 uv;

layout ( location = 0 ) out vec2 outUV;
layout ( location = 1 ) out vec3 outNormal;
layout ( location = 2 ) out vec3 outFragPosition;

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
    float linear;
    float quadratic;
};
layout ( std430, buffer_reference ) readonly buffer LightBuffer {
    PointLight pointLights[];
};

layout ( push_constant ) uniform PushConstants {
    mat4        mvp;
    uint        textureID;
    uint        lightCount;
    LightBuffer lightBuffer;
    vec3        cameraPosition;
} pc;

void main() {
    gl_Position     = pc.mvp * vec4( pos, 1.0f );
    outUV           = uv;
    outNormal       = normal;
    outFragPosition = pos;
}
