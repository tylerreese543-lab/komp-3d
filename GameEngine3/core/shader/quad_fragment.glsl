#version 330 core

out vec4 g_Diffuse;

uniform sampler2D t_Diffuse;
in vec2 a_UV;
uniform float u_CurrentTime;
void main() {

	//color = vec4(texture(g_Diffuse, a_UV).rgb, 1.);
	g_Diffuse = vec4(texture(t_Diffuse, a_UV).rgb, 1.);
}