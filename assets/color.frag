#version 330 core

in vec3 oColor;
out vec4 fColor;

void main() {
	fColor = vec4(oColor.rgb, 1.0);
};
