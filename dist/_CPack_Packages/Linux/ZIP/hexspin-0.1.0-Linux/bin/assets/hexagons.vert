#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in mat4 aModel;

uniform mat4 uView;
uniform mat4 uProj;
uniform float uRadius;
uniform samplerBuffer uColors;

out vec3 oColor;

vec3 rgb2hsv(vec3 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
	vec4 scale = vec4(uRadius, uRadius, 1, 1);
	
	gl_Position = uProj * uView * (aModel * vec4(aPos, 1.0) * scale);
	oColor = texelFetch(uColors, gl_InstanceID * 8 + gl_VertexID).rgb;
}
