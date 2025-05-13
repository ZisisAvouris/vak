
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require

struct DrawParameters {
    uint transformID;
    uint materialID;
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
layout ( std430, buffer_reference ) readonly buffer LightBuffer {
    PointLight pointLights[];
};

layout ( std430, buffer_reference ) readonly buffer TransformBuffer {
    mat4 model[];
};

struct TransparentFragment {
    f16vec4 color;
    float   depth;
    uint    next;
};
layout ( std430, buffer_reference ) buffer TransparencyList {
    TransparentFragment fragments[];
};

layout ( std430, buffer_reference ) buffer AtomicCounter {
    uint counter;
};

layout ( std430, buffer_reference ) buffer OITBuffer {
    AtomicCounter    atomicCounter;
    TransparencyList list;
    uint             texHeads;
    uint             maxTransparentFragments;
};

layout ( push_constant ) uniform PushConstants {
    mat4                viewProj;
    LightBuffer         lightBuffer;
    uint                lightCount;
    vec3                cameraPosition;
    TransformBuffer     transforms;
    DrawParameterBuffer drawParams;
    OITBuffer           oit;
} pc;

