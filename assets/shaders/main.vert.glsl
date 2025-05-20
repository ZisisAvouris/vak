#version 460 core
#extension GL_ARB_shading_language_include : require

#include "common.glsl"

layout ( location = 0 ) in vec3 inPos;
layout ( location = 1 ) in vec3 inNormal;
layout ( location = 2 ) in vec3 inTangent;
layout ( location = 3 ) in vec2 inUV;

layout ( location = 0 ) out vec2 outUV;
layout ( location = 1 ) out vec3 outFragPosition;
layout ( location = 2 ) out vec3 outViewPosition;
layout ( location = 3 ) out flat uint outBaseColorID;
layout ( location = 4 ) out flat uint outNormalID;
layout ( location = 5 ) out flat uint outMetallicRoughnessID;
layout ( location = 6 ) out mat3 outTBN;

void main() {
    const mat4 modelMatrix  = pc.transforms.model[pc.drawParams.dp[gl_BaseInstance].transformID];
    const mat3 normalMatrix = transpose( inverse( mat3( modelMatrix ) ) );

    vec3 tangent = normalize( normalMatrix * inTangent );
    vec3 normal  = normalize( normalMatrix * inNormal );
    tangent = normalize( tangent - dot( tangent, normal ) * normal );
    vec3 bitangent = cross( normal, tangent );
    outTBN = mat3( tangent, bitangent, normal );

    const vec4 tpos = modelMatrix * vec4( inPos, 1.0f );
    outUV           = inUV;
    outFragPosition = tpos.xyz;
    outViewPosition = pc.cameraPosition;
    outBaseColorID  = pc.drawParams.dp[gl_BaseInstance].baseColorID;
    outNormalID     = pc.drawParams.dp[gl_BaseInstance].normalID;
    outMetallicRoughnessID = pc.drawParams.dp[gl_BaseInstance].metallicRoughnessID;
    gl_Position     = pc.viewProj * tpos;
}
