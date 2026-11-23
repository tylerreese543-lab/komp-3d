#include "Camera.h"

#include "Graphics/GraphicsWrapper.h"
#include "Graphics/graphics_include.h"
#include "Graphics/Buffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Scene.h"
Camera::Camera() : SpatialInstance() {

	near_plane = 0;
	far_plane = 0;
	aspect_ratio = 1;
	name = "Camera";
	type = InstanceType_Camera;
	setPlanes(0.1, 1000.0f);
	fov = 90;
	isOrthographic = false;
}


Camera::Camera(vec3 v) : Camera() {
	setPosition(v.X, v.Y, v.Z);
}
void Camera::updateMatrices() {

	if (isOrthographic) {
		setMat4Row(projection.m[0],
			fovGL() * aspect_ratio, 0, 0, 0
		);
		setMat4Row(projection.m[1],
			0, fovGL(), 0, 0
		);
		setMat4Row(projection.m[2],
			0, 0, 1.0 / (far_plane - near_plane), -near_plane / (far_plane - near_plane)
		);
		setMat4Row(projection.m[3],
			0, 0, 0, 1.0f
		);
	}
	else {
		setMat4Row(projection.m[0],
			aspect_ratio * fovGL(), 0, 0, 0
		);
		setMat4Row(projection.m[1],
			0, fovGL(), 0, 0
		);
		setMat4Row(projection.m[2],
			0, 0, far_plane / (far_plane - near_plane), -(far_plane * near_plane) / (far_plane - near_plane)
		);
		setMat4Row(projection.m[3],
			0, 0, 1, 0
		);
	}

	vec3 rotation = getWorldRotation();
	view = mat3::yxz(-rotation.X, -rotation.Y, -rotation.Z);
	look = mat3::zxy(rotation.X, rotation.Y, rotation.Z);
	moveDirection2D = mat3::yrot(rotation.Y);
}

float Camera::fovGL() {
	return 1 / tanf(fov * (PI / 180.0f) * 0.5f);
}

void Camera::setPlanes(float near, float far) {
	near_plane = near;
	far_plane = far;
}

void Camera::setPosition(float x, float y, float z) {
	position = vec3(x, y, z);
}

int Camera::saveInstanceToFile(FILE* file) {
	SpatialInstance::saveInstanceToFile(file);
	fwrite(&near_plane, sizeof(float), 1, file);
	fwrite(&far_plane, sizeof(float), 1, file);
	fwrite(&fov, sizeof(float), 1, file);
	fwrite(&aspect_ratio, sizeof(float), 1, file);
	fwrite(&isOrthographic, sizeof(bool), 1, file);
	return 0; //TODO: return the size of the data written to the file
}

int Camera::loadInstanceFromFile(void* data, int off) {
	int o = off;
	o += SpatialInstance::loadInstanceFromFile(data, o); //Load the base instance data first
	near_plane = *(float*)offset(data, o);
	o += sizeof(float);
	far_plane = *(float*)offset(data, o);
	o += sizeof(float);
	fov = *(float*)offset(data, o);
	o += sizeof(float);
	aspect_ratio = *(float*)offset(data, o);
	o += sizeof(float);
	isOrthographic = *(bool*)offset(data, o);
	o += sizeof(bool);
	updateMatrices();
	return o - off; // Return the number of bytes read
}

Instance* Camera::clone_this() {
	Camera* newCamera = (Camera*)registerInstance(new Camera(), true);
	newCamera->name = this->name;
	newCamera->setParent(this->parent);
	newCamera->position = this->position;
	newCamera->rotation = this->rotation;
	newCamera->near_plane = this->near_plane;
	newCamera->far_plane = this->far_plane;
	newCamera->fov = this->fov;
	newCamera->aspect_ratio = this->aspect_ratio;
	newCamera->isOrthographic = this->isOrthographic;
	newCamera->updateMatrices();
	return newCamera;
}

void Camera::update(float deltaTime, bool isStudio) {
	updateMatrices();
	aspect_ratio = Graphics::getInstance()->getAspectRatio();
}

int Camera::getNumSaveBytes() {
	return sizeof(float) * 4 + SpatialInstance::getNumSaveBytes() + sizeof(bool);
}


void Camera::debugRender() {
		//GL_LINES
	Buffer vertexBuffer;

	vertexBuffer.data = new vec3[16]{
		//Back
		vec3(0, 0, 0),
		vec3(1, -1, 1),

		vec3(0, 0, 0),
		vec3(1, 1, 1),

		vec3(0, 0, 0),
		vec3(-1, 1, 1),

		vec3(0, 0, 0),
		vec3(-1, -1, 1),

		vec3(1, -1, 1),
		vec3(1, 1, 1),

		vec3(1, 1, 1),
		vec3(-1, 1, 1),

		vec3(-1, 1, 1),
		vec3(-1, -1, 1),

		vec3(-1, -1, 1),
		vec3(1, -1, 1),
	};

	vertexBuffer.element_size = sizeof(vec3);
	vertexBuffer.calcSize(16);
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
	mat4 transform = getWorldTransform();
	program->setUniformSMat(program->shaderUniformNames[eSUniform_ModelTransformMatrix], transform, GL_TRUE, 4);
	program->setUniform3f(program->shaderUniformNames[eSUniform_CameraPosition], camera->getWorldPosition());
	program->setUniformSMat(program->shaderUniformNames[eSUniform_CameraLookMatrix], camera->view, GL_TRUE, 3);
	program->setUniformSMat(program->shaderUniformNames[eSUniform_CameraProjectionMatrix], camera->projection, GL_TRUE, 4);
	program->setUniform1f(program->shaderUniformNames[eSUniform_CurrentTime], Graphics::getInstance()->lastFrameTime);
	program->setUniform3f(program->shaderUniformNames[eSUniform_Material_DiffuseColor], Graphics::debugDefaultColor);
	glBindVertexArray(vao);
	glDrawArrays(GL_LINES, 0, 16);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);


}