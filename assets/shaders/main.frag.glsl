#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

layout ( location = 0 ) in vec2 inUV;
layout ( location = 1 ) in vec3 inNormal;
layout ( location = 2 ) in vec3 inFragPosition;

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

layout ( push_constant ) uniform PushConstants {
    mat4        mvp;
    uint        textureID;
    uint        lightCount;
    LightBuffer lightBuffer;
    vec3        cameraPosition;
} pc;

layout ( set = 0, binding = 0 ) uniform texture2D bTextures[];
layout ( set = 0, binding = 1 ) uniform sampler   bSamplers[];

vec4 Sample( uint textureID, uint samplerID, vec2 uv ) {
    return texture( nonuniformEXT( sampler2D( bTextures[textureID], bSamplers[samplerID] ) ), uv );
}

void main() {
    const vec3 normal  = normalize( inNormal );
    const vec3 viewDir = normalize( pc.cameraPosition - inFragPosition );
    const vec3 albedo  = Sample( pc.textureID, 0, inUV ).rgb;

    vec3 finalColor = vec3( 0.0f, 0.0f, 0.0f );
    for ( uint i = 0; i < pc.lightCount; ++i ) {
        PointLight pointLight = pc.lightBuffer.pointLights[i];

        const float dist        = length( pointLight.position - inFragPosition );
        const float attenuation = 1.0f / ( 1.0f + pointLight.linear * dist + pointLight.quadratic * dist * dist );

        const vec3 ambient = albedo * 0.1f * attenuation;

        const vec3  lightDir = normalize( pointLight.position - inFragPosition );
        const float diff     = max( dot( lightDir, normal ), 0.0f );
        const vec3  diffuse  = diff * albedo * pointLight.color * attenuation;

        const vec3  halfwayDir = normalize( lightDir + viewDir );
        const float spec       = pow( max( dot( normal, halfwayDir), 0.0f ), 32.0f );
        const vec3  specular   = pointLight.color * spec * attenuation;

        finalColor += ( ambient + diffuse + specular ) * pointLight.intensity;
    }
    fragColor = vec4( finalColor, 1.0f );
}
