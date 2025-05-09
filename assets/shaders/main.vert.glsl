#version 460 core
#extension GL_EXT_buffer_reference : require

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec3 normal;
layout ( location = 2 ) in vec2 uv;

layout ( location = 0 ) out vec2 outUV;
layout ( location = 1 ) out vec3 outNormal;
layout ( location = 2 ) out vec3 outFragPosition;
layout ( location = 3 ) out flat uint outMaterialID;

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

struct DrawParameters {
    uint transformID;
    uint materialID;
};
layout ( std430, buffer_reference ) readonly buffer DrawParameterBuffer {
    DrawParameters dp[];
};

layout ( push_constant ) uniform PushConstants {
    mat4                viewProj;
    LightBuffer         lightBuffer;
    uint                lightCount;
    vec3                cameraPosition;
    TransformBuffer     transforms;
    DrawParameterBuffer drawParams;
} pc;

void main() {
    const mat4 modelMatrix = pc.transforms.model[pc.drawParams.dp[ gl_BaseInstance ].transformID];
    gl_Position            = pc.viewProj * modelMatrix * vec4( pos, 1.0f );

    outUV           = uv;
    outNormal       = transpose( inverse( mat3( modelMatrix ) ) ) * normal;
    outFragPosition = pos;
    outMaterialID   = pc.drawParams.dp[ gl_BaseInstance ].materialID;
}
