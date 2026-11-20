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

struct VertexData {
	vec4 color;
	vec4 position;
	vec3 world_position;
	vec2 uv;
	vec3 normal;
};

#line 1 1
// --- [INJECT USER CODE HERE] ---

void main() {
	

	VertexData vertexData;

	vertex_main(vertexData);

	out_normal = vertexData.normal;
	out_color = vertexData.color;
	out_texCoord = vertexData.uv;
	out_Position = vertexData.world_position;
	gl_Position = vertexData.position;
	//gl_Position = c_ProjectionMatrix * vec4(c_LookMatrix * (vertexData.position - c_Position), 1.0);
}