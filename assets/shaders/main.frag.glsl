#version 460 core
#extension GL_ARB_shading_language_include : require

#include "common.glsl"

layout ( location = 0 ) in vec2 inUV;
layout ( location = 1 ) in vec3 inFragPosition;
layout ( location = 2 ) in vec3 inViewPosition;
layout ( location = 3 ) in flat uint inBaseColorID;
layout ( location = 4 ) in flat uint inNormalID;
layout ( location = 5 ) in mat3 inTBN;

layout ( location = 0 ) out vec4 fragColor;

layout ( set = 0, binding = 0 ) uniform texture2D bTextures[];
layout ( set = 0, binding = 1 ) uniform sampler   bSamplers[];

vec4 Sample( uint textureID, uint samplerID, vec2 uv ) {
    return texture( nonuniformEXT( sampler2D( bTextures[textureID], bSamplers[samplerID] ) ), uv );
}

vec3 ComputePointLight( PointLight light, vec3 albedo, vec3 normal, vec3 viewDir ) {
    const float dist        = length( light.position - inFragPosition );
    const float attenuation = 1.0f / ( 1.0f + light.linear * dist + light.quadratic * dist * dist );

    const vec3 ambient = albedo * 0.1f * attenuation;

    const vec3  lightDir = normalize( inTBN * (light.position - inFragPosition) );
    const float diff     = max( dot( lightDir, normal ), 0.0f );
    const vec3  diffuse  = diff * albedo * light.color * attenuation;

    const vec3  halfwayDir = normalize( lightDir + viewDir );
    const float spec       = pow( max( dot( normal, halfwayDir), 0.0f ), 128.0f );
    const vec3  specular   = light.color * spec * attenuation;

    return ambient + diffuse + specular;
}

void main() {
    const vec3 normal  = normalize( Sample( inNormalID, 0, inUV ).rgb * 2.0f - 1.0f );
    const vec3 viewDir = normalize( inTBN * (inViewPosition - inFragPosition) );
    const vec3 albedo  = Sample( inBaseColorID, 0, inUV ).rgb;

    vec3 finalColor = vec3( 0.0f );
    for ( uint i = 0; i < pc.lightCount; ++i ) {
        PointLight pointLight = pc.lightBuffer.pointLights[i];
        finalColor += ComputePointLight( pointLight, albedo, normal, viewDir ) * pointLight.intensity;
    }
    fragColor = vec4( finalColor, 1.0f );
}
