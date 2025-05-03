#version 460 core
#extension GL_EXT_nonuniform_qualifier : require

layout ( location = 0 ) in vec2 inUV;

layout ( location = 0 ) out vec4 fragColor;

layout ( set = 0, binding = 0 ) uniform texture2D bTextures[];
layout ( set = 0, binding = 1 ) uniform sampler   bSamplers[];

vec4 Sample( uint textureID, uint samplerID, vec2 uv ) {
    return texture( nonuniformEXT( sampler2D( bTextures[textureID], bSamplers[samplerID] ) ), uv );
}

void main() {
    fragColor = Sample( 0, 0, inUV );
}
