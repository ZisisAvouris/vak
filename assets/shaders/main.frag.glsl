#version 460 core
#extension GL_EXT_nonuniform_qualifier : require

layout ( location = 0 ) in vec2 inUV;
layout ( location = 1 ) in flat uint textureID;

layout ( location = 0 ) out vec4 fragColor;

layout ( set = 0, binding = 0 ) uniform texture2D bTextures[];
layout ( set = 0, binding = 1 ) uniform sampler   bSamplers[];

vec4 Sample( uint textureID, uint samplerID, vec2 uv ) {
    return texture( nonuniformEXT( sampler2D( bTextures[textureID], bSamplers[samplerID] ) ), uv );
}

void main() {
    fragColor = Sample( textureID, 0, inUV );
    // fragColor = vec4( clamp(inUV.x, 0.0f, 1.0f), clamp(inUV.y, 0.0f, 1.0f), 0.5f, 1.0f);
}
