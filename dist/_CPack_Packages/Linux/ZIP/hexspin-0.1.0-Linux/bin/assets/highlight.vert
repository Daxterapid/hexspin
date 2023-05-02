#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uTranslate;
uniform float uRadius;

out vec3 oColor;

void main() {
	gl_Position = uProj * uView * vec4((aPos + uTranslate) * uRadius, 1);
	oColor = vec3(1.0);
}
