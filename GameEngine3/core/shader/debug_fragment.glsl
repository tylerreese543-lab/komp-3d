#version 330 core

out vec4 color;

uniform vec3 m_DiffuseColor;
void main() {
	color = vec4(m_DiffuseColor, 1.);
}