#version 330 core
in vec4 m_Color;
in vec3 m_Position;
in vec2 m_TexCoord;
in vec3 m_Normal;
uniform mat3 c_LookMatrix;
uniform vec3 c_Position;
uniform mat4 c_ProjectionMatrix;
uniform mat4 m_Transform;
out vec4 out_color;
out vec2 out_texCoord;
out vec3 out_normal;
out vec3 out_Position;
void main() {

	vec3 mesh_pos = (m_Transform * vec4(m_Position, 1.0)).xyz;
	vec3 mesh_norm = transpose(inverse(mat3(m_Transform))) * m_Normal;

	out_normal = mesh_norm;
	out_color = m_Color;
	out_texCoord = m_TexCoord;
	out_Position = mesh_pos;
	gl_Position = c_ProjectionMatrix * vec4(c_LookMatrix * (mesh_pos - c_Position), 1.0);
}