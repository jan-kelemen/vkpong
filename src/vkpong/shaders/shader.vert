#version 450

layout(push_constant) uniform PushConsts {
	vec4 color[6];
} pushConsts;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inOffset;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition - inOffset, 0.0, 1.0);
    fragColor = vec3(pushConsts.color[gl_VertexIndex]);
}
