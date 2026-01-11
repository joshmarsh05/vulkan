#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vVertex;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec2 texCoords;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 projectionMatrix;
    mat4 viewMatrix;
} camera;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat3x4 normalMatrix;
    uint colorID;
} push;

layout (location = 1) flat out uint color;


void main() {
    color = push.colorID;
    gl_Position = camera.projectionMatrix * camera.viewMatrix * push.modelMatrix * vVertex;
}