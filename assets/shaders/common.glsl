
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require

struct DrawParameters {
    uint transformID;
    uint baseColorID;
    uint normalID;
};
layout ( std430, buffer_reference ) readonly buffer DrawParameterBuffer {
    DrawParameters dp[];
};

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
    float linear;
    float quadratic;
};
layout ( std430, buffer_reference ) buffer LightBuffer {
    PointLight pointLights[];
};

layout ( std430, buffer_reference ) readonly buffer TransformBuffer {
    mat4 model[];
};

layout ( push_constant ) uniform PushConstants {
    mat4                viewProj;
    LightBuffer         lightBuffer;
    uint                lightCount;
    vec3                cameraPosition;
    TransformBuffer     transforms;
    DrawParameterBuffer drawParams;
} pc;

