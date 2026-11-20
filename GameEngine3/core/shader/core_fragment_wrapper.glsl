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


struct FragmentData {
	vec4 diffuse;
	vec3 normal;
	vec3 position;
	float specular;
	float roughness;
};
#line 1 1
// --- [INJECT USER CODE HERE] ---

void main() {
	
	FragmentData fragmentData;
	fragment_main(fragmentData);

	g_Specular = fragmentData.specular;
	g_Roughness = fragmentData.roughness;
	g_Diffuse = fragmentData.diffuse;
	g_Normal = fragmentData.normal;
	g_Position = fragmentData.position;
}