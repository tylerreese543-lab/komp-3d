#pragma once
#include "GameInstance.h"
#include "Graphics/matrix.h"
#include "Graphics/Buffer.h"
#include "Shader.h"
struct LightData {
	vec4 position;
	vec4 color; //xyz = rgb, w = intensity

	LightData();
};
class Light : public Instance
{
	static std::map<int, LightData> lights;
public:

	static vec3 ambient;
	static Buffer ubo;
	static int numLights;
	const static int maxLights;
	Light();
	~Light();

	LightData* getLightData();

	void setPosition(vec3 position);
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
};


