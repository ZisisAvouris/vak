#version 460 core

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec2 uv;

layout ( location = 0 ) out vec2 outUV;
layout ( location = 1 ) out flat uint outTexID;

layout ( push_constant ) uniform PushConstants {
    mat4 mvp;
    uint textureID;
} pc;

void main() {
    gl_Position = pc.mvp * vec4( pos, 1.0f );
    outUV       = uv;
    outTexID    = pc.textureID;
}
