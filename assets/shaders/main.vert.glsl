#version 460 core
#extension GL_ARB_shading_language_include : require

#include "common.glsl"

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec3 normal;
layout ( location = 2 ) in vec3 tangent;
layout ( location = 3 ) in vec2 uv;

layout ( location = 0 ) out vec2 outUV;
layout ( location = 1 ) out vec3 outFragPosition;
layout ( location = 2 ) out vec3 outViewPosition;
layout ( location = 3 ) out flat uint outBaseColorID;
layout ( location = 4 ) out flat uint outNormalID;
layout ( location = 5 ) out mat3 outTBN;

void main() {
    const mat4 modelMatrix = pc.transforms.model[pc.drawParams.dp[gl_BaseInstance].transformID];

    const mat3 normalMatrix = transpose( inverse( mat3( modelMatrix ) ) );
    vec3 T = normalize( normalMatrix * tangent );
    vec3 N = normalize( normalMatrix * normal );
    T = normalize( T - dot( T, N ) * N );
    vec3 B = cross( N, T );
    outTBN = mat3( T, B, N );

    const vec4 tpos = modelMatrix * vec4( pos, 1.0f );
    outUV           = uv;
    outFragPosition = vec3( tpos );
    outViewPosition = pc.cameraPosition;
    outBaseColorID  = pc.drawParams.dp[gl_BaseInstance].baseColorID;
    outNormalID     = pc.drawParams.dp[gl_BaseInstance].normalID;
    gl_Position     = pc.viewProj * tpos;
}
