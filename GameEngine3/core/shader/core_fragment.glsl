#version 330 core

in vec4 out_color;
in vec2 out_texCoord;
in vec3 out_normal;
in vec3 out_Position;
out vec4 g_Diffuse;

uniform sampler2D t_Diffuse;
uniform sampler2D t_Roughness;
uniform bool m_HasDiffuseTexture;
uniform sampler2D t_AmbientOcclusion;
uniform bool m_HasAmbientOcclusionTexture;
uniform bool m_HasRoughnessTexture;
uniform vec4 m_DiffuseColor;
uniform bool m_HasSpecularTexture;
uniform sampler2D t_Specular;
uniform float m_Roughness;
uniform int m_Specularity;
out vec3 g_Normal;
out vec3 g_Position;
out float g_Specular;
out float g_Roughness;
void main() {
	
	vec3 col = m_DiffuseColor.rgb;
	if (m_HasDiffuseTexture) {
		col *= texture(t_Diffuse, out_texCoord).rgb;
	}
	float AOValue = 1.;
	if (m_HasAmbientOcclusionTexture) {
		AOValue = texture(t_AmbientOcclusion, out_texCoord).r;
	}
	float rough = m_Roughness;
	if (m_HasRoughnessTexture) {
		rough *= texture(t_Roughness, out_texCoord).r;
	}

	float spec = float(m_Specularity);
	if (m_HasSpecularTexture) {
		spec *= texture(t_Specular, out_texCoord).r;
	}

	g_Specular = spec;
	g_Roughness = rough;
	g_Diffuse = vec4(col * AOValue, m_DiffuseColor.a);
	g_Normal = out_normal;
	g_Position = out_Position;
}