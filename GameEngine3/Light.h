#pragma once
#include "GameInstance.h"
#include "Graphics/matrix.h"
#include "Graphics/Buffer.h"
#include "Shader.h"
#include "SpatialInstance.h"
struct LightData {
	vec4 position;
	vec4 color; //xyz = rgb, w = intensity
	LightData();
};

struct DirectionalLightData {
	vec4 position;
	vec4 color;
	vec4 direction; //xyz = normal, w = range (-1 -> 1, -1 always for sun lights);
	DirectionalLightData();
};
class Light : public SpatialInstance
{
	static std::set<int> lights;
public:

	static vec3 ambient;
	static Buffer uboPointLights;
	static Buffer uboDirectionalLights;
	static int numPointLights;
	static int numDirectionalLights;
	const static int maxLights;
	Light();
	~Light();
	
	vec3 color;
	float intensity;
	float rangeAngle;

	//Whether the light is a sun light.
	bool isSunLight;
	//Whether the light is directional;
	bool isDirectional;
	LightData packPoint();
	DirectionalLightData packDirectional();
	void setColor(vec3 color);
	void setIntensity(float intensity);
	static void InitializeLightingUBO();
	static void bindLightsToRender();
	static void updateLightsToUBO();

	virtual Instance* clone_this();
	virtual int saveInstanceToFile(FILE* file);
	virtual int loadInstanceFromFile(void* data, int offset = 0);
	virtual int getNumSaveBytes();

	static int serializeGlobalLightData(FILE* file);
	static int deserializeGlobalLightData(void* data, int offset = 0);
	static int getGlobalLightDataSize();

	virtual void makeActive();
	virtual void makeInactive();
	void debugRender();
};


