#include "Light.h"
#include "Graphics/Graphics.h"

std::map<int, LightData> Light::lights;
Buffer Light::ubo;
const int Light::maxLights = 32;
int Light::numLights = 0;
vec3 Light::ambient = vec3(0.1, 0.1, 0.1);
Light::Light() : Instance() {
	name = "Light";
	type = InstanceType_PointLight;

	lights.insert(std::pair<int, LightData>(getId(), LightData()));

	setPosition(vec3(0, 0, 0));
	setColor(vec3(1, 1, 1));
	setIntensity(1);
}

LightData::LightData() {

}
LightData* Light::getLightData() {
	return &lights[getId()];
}
void Light::setPosition(vec3 p) {
	getLightData()->position.X = p.X;
	getLightData()->position.Y = p.Y;
	getLightData()->position.Z = p.Z;
}
void Light::setColor(vec3 p) {
	getLightData()->color.X = p.X;
	getLightData()->color.Y = p.Y;
	getLightData()->color.Z = p.Z;
}
void Light::setIntensity(float p) {
	getLightData()->color.W = p;
}
void Light::InitializeLightingUBO() {
	ubo.target = GL_UNIFORM_BUFFER;
	ubo.generate();
	ubo.data = new LightData[maxLights];
	ubo.element_size = sizeof(LightData);
	
}

Light::~Light() {
	lights.erase(getId());
}
void Light::updateLightsToUBO() {
	
	int i = 0;
	for (auto pair : lights) {
		if (i >= maxLights) {
			break;
		}

		((LightData*)ubo.data)[i] = pair.second;
		i++;
	}
	numLights = i;
	ubo.calcSize(i);
	ubo.update();


}

void Light::bindLightsToRender() {
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo.id);
}

Instance* Light::clone_this() {
	Light* p = new Light();

	p->getLightData()->position = getLightData()->position;
	p->getLightData()->color = getLightData()->color;
	p->name = name;
	p->setParent(parent);

	return p;
}

int Light::saveInstanceToFile(FILE* file) {
	Instance::saveInstanceToFile(file);
	fwrite(&getLightData()->position, sizeof(vec4), 1, file);
	fwrite(&getLightData()->color, sizeof(vec4), 1, file);
	return 0;
}

int Light::loadInstanceFromFile(void* data, int off) {
	int o = off;
	o += Instance::loadInstanceFromFile(data, o);
	vec4 pos = *(vec4*)offset(data, o);
	o += sizeof(vec4);
	vec4 col = *(vec4*)offset(data, o);
	o += sizeof(vec4);
	setPosition(vec3(pos.X, pos.Y, pos.Z));
	setColor(vec3(col.X, col.Y, col.Z));
	setIntensity(col.W);
	return o - off;
}

int Light::getNumSaveBytes() {
	return Instance::getNumSaveBytes() + sizeof(vec4) * 2;
}

int Light::serializeGlobalLightData(FILE* file) {	
	int offset = 0;
	offset += fwrite(&ambient, sizeof(vec3), 1, file);
	offset += fwrite(&Graphics::backgroundColor, sizeof(vec4), 1, file);
	return offset;
}

int Light::deserializeGlobalLightData(void* data, int off) {
	int o = off;
	ambient = *(vec3*)offset(data, o);
	o += sizeof(vec3);
	Graphics::backgroundColor = *(vec4*)offset(data, o);
	o += sizeof(vec4);
	return o - off;
}

int Light::getGlobalLightDataSize() {
	return sizeof(vec3) + sizeof(vec4);
}