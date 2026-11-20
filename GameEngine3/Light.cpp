#include "Light.h"
#include "Graphics/Graphics.h"


#include "Graphics/graphics_include.h"
#include "Graphics/Buffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Scene.h"
std::set<int> Light::lights;
Buffer Light::uboPointLights;
Buffer Light::uboDirectionalLights;
const int Light::maxLights = 32;
int Light::numPointLights = 0;
int Light::numDirectionalLights = 0;
vec3 Light::ambient = vec3(0.1, 0.1, 0.1);
Light::Light() : SpatialInstance() {
	name = "Light";
	type = InstanceType_PointLight;

	lights.insert(getId());


	rangeAngle = 1.0f;
	isSunLight = false;
	isDirectional = false;
	setColor(vec3(1, 1, 1));
	setIntensity(1);
}

LightData::LightData() {

}
DirectionalLightData::DirectionalLightData() {

}
LightData Light::packPoint() {
	LightData data;
	vec3 worldPos = getWorldPosition();
	data.position = vec4(worldPos.X, worldPos.Y, worldPos.Z, 1.0f);
	data.color = vec4(color.X, color.Y, color.Z, intensity);
	return data;
}
DirectionalLightData Light::packDirectional() {
	DirectionalLightData data;
	vec3 worldPos = getWorldPosition();
	vec3 worldRot = getWorldRotation();
	vec3 look = mat4::yxz(worldRot.X, worldRot.Y, worldRot.Z) * vec3(0, 0, -1);
	data.position = vec4(worldPos.X, worldPos.Y, worldPos.Z, 1.0f);
	data.color = vec4(color.X, color.Y, color.Z, intensity);
	data.direction = vec4(look.X, look.Y, look.Z, isSunLight ? -1.0f : cosf(rangeAngle));
	return data;
}
void Light::setColor(vec3 p) {
	color = p;
}
void Light::setIntensity(float p) {
	intensity = p;
}
void Light::InitializeLightingUBO() {
	uboPointLights.target = GL_UNIFORM_BUFFER;
	uboPointLights.generate();
	uboPointLights.data = new LightData[maxLights];
	uboPointLights.element_size = sizeof(LightData);
	
	uboDirectionalLights.target = GL_UNIFORM_BUFFER;
	uboDirectionalLights.generate();
	uboDirectionalLights.data = new DirectionalLightData[maxLights];
	uboDirectionalLights.element_size = sizeof(DirectionalLightData);
}

Light::~Light() {
	lights.erase(getId());
}
void Light::updateLightsToUBO() {
	
	int i = 0;
	for (int id : lights) {
		if (i >= maxLights) {
			break;
		}

		if (!getInstanceById(id)) continue;

		Light* lightInstance = dynamic_cast<Light*>(getInstanceById(id));

		if (lightInstance->isDirectional || lightInstance->isSunLight) continue;
		if (!isInstanceOf(lightInstance->type, Instance::InstanceType_PointLight)) continue;
		((LightData*)uboPointLights.data)[i] = lightInstance->packPoint();
		i++;
	}
	numPointLights = i;
	uboPointLights.calcSize(i);
	uboPointLights.update();


	i = 0;
	for (int id : lights) {
		if (i >= maxLights) {
			break;
		}

		if (!getInstanceById(id)) continue;

		Light* lightInstance = dynamic_cast<Light*>(getInstanceById(id));
		if (!lightInstance->isDirectional && !lightInstance->isSunLight) continue;
		if (!isInstanceOf(lightInstance->type, Instance::InstanceType_PointLight)) continue;
		((DirectionalLightData*)uboDirectionalLights.data)[i] = lightInstance->packDirectional();
		i++;
	}
	numDirectionalLights = i;
	uboDirectionalLights.calcSize(i);
	uboDirectionalLights.update();
}

void Light::bindLightsToRender() {
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboPointLights.id);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboDirectionalLights.id);
}

Instance* Light::clone_this() {
	Light* p = (Light*)registerInstance(new Light(*this), true);

	lights.insert(p->getId());
	return p;
}

int Light::saveInstanceToFile(FILE* file) {
	SpatialInstance::saveInstanceToFile(file);
	fwrite(&color, sizeof(vec3), 1, file);
	fwrite(&intensity, sizeof(float), 1, file);
	fwrite(&isDirectional, sizeof(bool), 1, file);
	fwrite(&isSunLight, sizeof(bool), 1, file);
	fwrite(&rangeAngle, sizeof(float), 1, file);
	return 0;
}

int Light::loadInstanceFromFile(void* data, int off) {
	int o = off;
	o += SpatialInstance::loadInstanceFromFile(data, o);
	vec3 col = *(vec3*)offset(data, o);
	o += sizeof(vec3);
	float inten = *(float*)offset(data, o);
	o += sizeof(float);
	setColor(col);
	setIntensity(inten);
	isDirectional = *(bool*)offset(data, o);
	o += sizeof(bool);
	isSunLight = *(bool*)offset(data, o);
	o += sizeof(bool);
	rangeAngle = *(float*)offset(data, o);
	o += sizeof(float);
	return o - off;
}

int Light::getNumSaveBytes() {
	return SpatialInstance::getNumSaveBytes() + (sizeof(vec3) * 1) + (2 * sizeof(float)) + (2 * sizeof(bool));
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

void Light::debugRender() {
	Buffer vertexBuffer;
	

	if (isDirectional || isSunLight) {
		vertexBuffer.data = new vec3[24]{
			// Top pyramid edges
			{ 0.0f,  1.0f,  0.0f}, {-1.0f,  0.0f,  0.0f}, // Top to Left
			{ 0.0f,  1.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, // Top to Right
			{ 0.0f,  1.0f,  0.0f}, { 0.0f,  0.0f,  2.0f}, // Top to Front
			{ 0.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, // Top to Back

			// Bottom pyramid edges
			{ 0.0f, -1.0f,  0.0f}, {-1.0f,  0.0f,  0.0f}, // Bottom to Left
			{ 0.0f, -1.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, // Bottom to Right
			{ 0.0f, -1.0f,  0.0f}, { 0.0f,  0.0f,  2.0f}, // Bottom to Front
			{ 0.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, // Bottom to Back

			// Middle ring edges
			{-1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f,  2.0f}, // Left to Front
			{ 0.0f,  0.0f,  2.0f}, { 1.0f,  0.0f,  0.0f}, // Front to Right
			{ 1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, // Right to Back
			{ 0.0f,  0.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}  // Back to Left
		};
	}
	else {
		vertexBuffer.data = new vec3[24]{
			// Top pyramid edges
			{ 0.0f,  1.0f,  0.0f}, {-1.0f,  0.0f,  0.0f}, // Top to Left
			{ 0.0f,  1.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, // Top to Right
			{ 0.0f,  1.0f,  0.0f}, { 0.0f,  0.0f,  1.0f}, // Top to Front
			{ 0.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, // Top to Back

			// Bottom pyramid edges
			{ 0.0f, -1.0f,  0.0f}, {-1.0f,  0.0f,  0.0f}, // Bottom to Left
			{ 0.0f, -1.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, // Bottom to Right
			{ 0.0f, -1.0f,  0.0f}, { 0.0f,  0.0f,  1.0f}, // Bottom to Front
			{ 0.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, // Bottom to Back

			// Middle ring edges
			{-1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f,  1.0f}, // Left to Front
			{ 0.0f,  0.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, // Front to Right
			{ 1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, // Right to Back
			{ 0.0f,  0.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}  // Back to Left
		};
	}

	vertexBuffer.element_size = sizeof(vec3);
	vertexBuffer.calcSize(24);
	unsigned int vao = 0;

	ShaderProgram* program = (ShaderProgram*)getInstanceById(Scene::debugShader);
	program->useProgram();
	vertexBuffer.generate();
	GenVAO(&vao, &vertexBuffer, nullptr, &program->layout);

	Camera* camera = Scene::getCurrentScene()->getCamera();

	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	// Draw call 1 - simulated (oriented) bounding box;
	program->setUniformSMat(program->shaderUniformNames[eSUniform_ModelTransformMatrix], getWorldTransform(), GL_TRUE, 4);
	program->setUniform3f(program->shaderUniformNames[eSUniform_CameraPosition], camera->getWorldPosition());
	program->setUniformSMat(program->shaderUniformNames[eSUniform_CameraLookMatrix], camera->look, GL_TRUE, 3);
	program->setUniformSMat(program->shaderUniformNames[eSUniform_CameraProjectionMatrix], camera->projection, GL_TRUE, 4);
	program->setUniform1f(program->shaderUniformNames[eSUniform_CurrentTime], Graphics::getInstance()->lastFrameTime);
	program->setUniform3f(program->shaderUniformNames[eSUniform_Material_DiffuseColor], Graphics::debugDefaultColor);
	glBindVertexArray(vao);
	glDrawArrays(GL_LINES, 0, 24);
	glBindVertexArray(0);
}

void Light::makeActive() {
	lights.insert(getId());
	SpatialInstance::makeActive();
}

void Light::makeInactive() {
	lights.erase(getId());
	SpatialInstance::makeInactive();
}