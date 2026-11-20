#version 330 core

#define MAX_LIGHTS 32
out vec4 g_Diffuse;

uniform sampler2D t_Diffuse;
uniform sampler2D t_Normal;
uniform sampler2D t_Position;
uniform sampler2D t_Roughness;
uniform sampler2D t_Specular;
in vec2 a_UV;
uniform float u_CurrentTime;

struct Light {
	vec4 position;
	vec4 color;
};

struct DirectionalLight {
	vec4 position;
	vec4 color;
    vec4 direction;
};
layout (std140) uniform PointLightData {
	Light plights[MAX_LIGHTS];
};
layout (std140) uniform DirectionalLightData {
	DirectionalLight dlights[MAX_LIGHTS];
};
uniform int l_NumPointLights;
uniform int l_NumDirectionalLights;
uniform vec3 l_AmbientLight;

uniform vec3 c_Position;
void main() {
	
	//color = vec4(texture(g_Diffuse, a_UV).rgb, 1.);


    vec3 fragPos = texture(t_Position, a_UV).xyz;
    vec3 normal = texture(t_Normal, a_UV).xyz;

    vec3 norm = normalize(normal);
    vec3 albedo = texture(t_Diffuse, a_UV).rgb;
	vec3 lighting = l_AmbientLight;

    float roughness = (1 - texture(t_Roughness, a_UV).r);
    float specularity = texture(t_Specular, a_UV).r;
    vec3 viewDir = normalize(c_Position - fragPos);

	for (int i = 0; i < l_NumPointLights; ++i) {
        vec3 lightPos = plights[i].position.xyz;
        vec3 lightColor = plights[i].color.rgb;
        float intensity = plights[i].color.w;

        // Diffuse calculations
        vec3 lightDir = normalize(lightPos - fragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor * intensity;

        // Specular calculations (Blinn-Phong)
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(norm, halfwayDir), 0.0), int(specularity));
        vec3 specular = spec * lightColor * roughness;

        // Distance Attenuation
        float distance = length(lightPos - fragPos);
        float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * (distance * distance));

        lighting += (diffuse + specular) * attenuation;
	}

    for (int i = 0; i < l_NumDirectionalLights; ++i) {
        vec3 lightPos = dlights[i].position.xyz;
        vec3 lightColor = dlights[i].color.rgb;
        float intensity = dlights[i].color.w;

        vec3 lightDirection = dlights[i].direction.xyz;
        float lightRange = dlights[i].direction.w;
        

        // Diffuse calculations
        vec3 lightDir = normalize(lightPos - fragPos);

        if (dot(lightDir, lightDirection) < lightRange && lightRange != -1.) {
            continue;
        }

        if (lightRange == -1.) {
            lightDir = lightDirection;
        }
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor * intensity;

        // Specular calculations (Blinn-Phong)
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(norm, halfwayDir), 0.0), int(specularity));
        vec3 specular = spec * lightColor * roughness;

        // Distance Attenuation
        float distance = length(lightPos - fragPos);
        float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * (distance * distance));

        if (lightRange == -1.) {
            attenuation = 1.;
        }
        lighting += (diffuse + specular) * attenuation;
	}
 	g_Diffuse = vec4(lighting * albedo, 1.);
}