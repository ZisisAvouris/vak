#version 460 core
#extension GL_ARB_shading_language_include : require

#include "common.glsl"

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec3 normal;
layout ( location = 2 ) in vec2 uv;

layout ( location = 0 ) out vec2 outUV;
layout ( location = 1 ) out vec3 outNormal;
layout ( location = 2 ) out vec3 outFragPosition;
layout ( location = 3 ) out flat uint outMaterialID;

void main() {
    const mat4 modelMatrix = pc.transforms.model[pc.drawParams.dp[ gl_BaseInstance ].transformID];
    gl_Position            = pc.viewProj * modelMatrix * vec4( pos, 1.0f );

    outUV           = uv;
    outNormal       = transpose( inverse( mat3( modelMatrix ) ) ) * normal;
    outFragPosition = pos;
    outMaterialID   = pc.drawParams.dp[ gl_BaseInstance ].materialID;
}
