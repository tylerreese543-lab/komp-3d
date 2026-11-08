#version 330 core
in vec3 m_Position;
in vec2 m_TexCoord;
out vec2 a_UV;
uniform mat4 m_Transform;

void main() {
	a_UV = m_TexCoord;
	gl_Position = vec4(m_Position.xy, 0.0, 1.0);
}