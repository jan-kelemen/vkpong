#version 450

layout(push_constant) uniform PushConsts {
	vec4 color[6];
	vec2 resolution;
} pushConsts;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 inOffset;

layout(location = 0) out vec4 outColor;

void main() {
	float x = inOffset.x * pushConsts.resolution.x;
	float y = inOffset.y * pushConsts.resolution.y;

	float xdis = distance(x, gl_FragCoord.x - x);
	float ydis = distance(y, gl_FragCoord.y - y);

	if (sqrt(xdis * xdis + ydis * ydis) > 0.015 * pushConsts.resolution.x)
		discard;

    outColor = vec4(fragColor, 1.0);
}
