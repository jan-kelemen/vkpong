#version 450

layout(push_constant) uniform PushConsts {
	vec4 color[6];
} pushConsts;

layout(location = 0) in vec2 position;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    fragColor = vec3(pushConsts.color[gl_VertexIndex]);
}
