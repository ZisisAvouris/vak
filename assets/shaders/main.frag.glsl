#version 460 core
#extension GL_ARB_shading_language_include : require

#include "common.glsl"

layout ( location = 0 ) in vec2 inUV;
layout ( location = 1 ) in vec3 inFragPosition;
layout ( location = 2 ) in vec3 inViewPosition;
layout ( location = 3 ) in flat uint inBaseColorID;
layout ( location = 4 ) in flat uint inNormalID;
layout ( location = 5 ) in flat uint inMetallicRoughnessID;
layout ( location = 6 ) in mat3 inTBN;

layout ( location = 0 ) out vec4 fragColor;

layout ( set = 0, binding = 0 ) uniform texture2D bTextures[];
layout ( set = 0, binding = 1 ) uniform sampler   bSamplers[];

const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec4 Sample( uint textureID, uint samplerID, vec2 uv ) {
    return texture( nonuniformEXT( sampler2D( bTextures[textureID], bSamplers[samplerID] ) ), uv );
}

vec3 TonemapACES( vec3 color ) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    const vec3 normalSample = Sample( inNormalID, 0, inUV ).rgb * 2.0f - 1.0f;
    const vec3 normal       = normalize( inTBN * normalSample );

    const vec3 viewDir = normalize( inViewPosition - inFragPosition );
    const vec3 albedo  = pow( Sample( inBaseColorID, 0, inUV ).rgb, vec3( 2.2f ) );
    const vec3 metallicRoughness = Sample( inMetallicRoughnessID, 0, inUV ).rgb;
    const float metalness = metallicRoughness.b;
    const float roughness = metallicRoughness.g;

    const vec3 F0 = mix( vec3(0.04), albedo, metalness );

    vec3 Lo = vec3(0.0f);
    for ( uint i = 0; i < pc.lightCount; ++i ) {
        PointLight pointLight = pc.lightBuffer.pointLights[i];

        const vec3 L = normalize( pointLight.position - inFragPosition );
        const vec3 H = normalize( viewDir + L );
        const float NdotL = max( dot(normal, L), 0.0f );

        const float dist = length( pointLight.position - inFragPosition );
        const float attenuation = 1.0f / ( 1.0f + pointLight.linear * dist + pointLight.quadratic * dist * dist );

        const vec3 radiance = pointLight.color * attenuation;

        const float NDF = DistributionGGX( normal, H, roughness );
        const float G   = GeometrySmith( normal, viewDir, L, roughness );
        const vec3  F   = fresnelSchlick( max( dot(H, viewDir), 0.0f ), F0);

        const vec3  numerator   = NDF * G * F;
        const float denominator = 4.0f * max( dot(normal, viewDir), 0.0f ) * NdotL + 0.0001f;
        const vec3  specular    = numerator / denominator;

        const vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;
        kD *= 1.0f - metalness;

        Lo += ( kD * albedo / PI + specular ) * radiance * NdotL * pointLight.intensity;
    }

    const vec3 ambient = albedo * 0.03f * ( 1.0f - metalness );
    vec3 finalColor = pow( TonemapACES( ambient + Lo ), vec3( 1.0f / 2.2f ) );
    fragColor = vec4( finalColor, 1.0f );
}
