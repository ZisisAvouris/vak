#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

layout ( location = 0 ) in vec2 inUV;
layout ( location = 1 ) in vec3 inNormal;
layout ( location = 2 ) in vec3 inFragPosition;
layout ( location = 3 ) in flat uint inMaterialID;

layout ( location = 0 ) out vec4 fragColor;

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

layout ( set = 0, binding = 0 ) uniform texture2D bTextures[];
layout ( set = 0, binding = 1 ) uniform sampler   bSamplers[];

vec4 Sample( uint textureID, uint samplerID, vec2 uv ) {
    return texture( nonuniformEXT( sampler2D( bTextures[textureID], bSamplers[samplerID] ) ), uv );
}

vec3 ComputeDirectionalLight( vec3 albedo, vec3 normal, vec3 viewDir ) {
    const vec3 lightColor = vec3( 1.0f, 1.0f, 1.0f );
    const vec3 lightDir = normalize( -vec3( -0.2f, -1.0f, -0.3f ) );
    const vec3 ambient  = albedo * lightColor;
    
    const float diff    = max( dot( normal, lightDir ), 0.0f );
    const vec3  diffuse = albedo * diff * lightColor;

    const vec3  reflectDir = reflect( -lightDir, normal );
    const float spec       = pow( max( dot( viewDir, reflectDir ), 0.0f ), 32.0f );
    const vec3  specular   = albedo * spec * lightColor;

    return diffuse + specular;
}

vec3 ComputePointLight( PointLight light, vec3 albedo, vec3 normal, vec3 viewDir ) {
    const float dist        = length( light.position - inFragPosition );
    const float attenuation = 1.0f / ( 1.0f + light.linear * dist + light.quadratic * dist * dist );

    const vec3 ambient = albedo * 0.1f * attenuation;

    const vec3  lightDir = normalize( light.position - inFragPosition );
    const float diff     = max( dot( lightDir, normal ), 0.0f );
    const vec3  diffuse  = diff * albedo * light.color * attenuation;

    const vec3  halfwayDir = normalize( lightDir + viewDir );
    const float spec       = pow( max( dot( normal, halfwayDir), 0.0f ), 32.0f );
    const vec3  specular   = light.color * spec * attenuation;

    return ambient + diffuse + specular;
}

void main() {
    const vec3 normal  = normalize( inNormal );
    const vec3 viewDir = normalize( pc.cameraPosition - inFragPosition );
    const vec3 albedo  = Sample( inMaterialID, 0, inUV ).rgb;

    vec3 finalColor = ComputeDirectionalLight( albedo, normal, viewDir );
    for ( uint i = 0; i < pc.lightCount; ++i ) {
        PointLight pointLight = pc.lightBuffer.pointLights[i];
        finalColor += ComputePointLight( pointLight, albedo, normal, viewDir ) * pointLight.intensity;
    }
    fragColor = vec4( finalColor, 1.0f );
}
