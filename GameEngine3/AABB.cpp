#include "AABB.h"

#include "Graphics/graphics_include.h"
#include "Graphics/Buffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Scene.h"
std::set<int> Collision3D::allCollisionBoxes;
AABB2D::AABB2D() {
	min = vec2(0.0f, 0.0f);
	max = vec2(0.0f, 0.0f);
}

AABB2D::AABB2D(const vec2& min, const vec2& max) {
	this->min = min;
	this->max = max;
}

bool AABB2D::intersects(const AABB2D& other) const {
	return (min.X <= other.max.X && max.X >= other.min.X) &&
		(min.Y <= other.max.Y && max.Y >= other.min.Y);
}

bool AABB2D::contains(const vec2& point) const {
	printf("Checking if point (%f, %f) is inside AABB2D with min (%f, %f) and max (%f, %f)\n", point.X, point.Y, min.X, min.Y, max.X, max.Y);
	return (point.X >= min.X && point.X <= max.X) &&
		(point.Y >= min.Y && point.Y <= max.Y);
}

AABB3D::AABB3D() {
	min = vec3(0.0f, 0.0f, 0.0f);
	max = vec3(0.0f, 0.0f, 0.0f);
}

AABB3D::AABB3D(const vec3& min, const vec3& max) {
	this->min = min;
	this->max = max;
}

bool AABB3D::intersects(const AABB3D& other) const {
	return (min.X <= other.max.X && max.X >= other.min.X) &&
		(min.Y <= other.max.Y && max.Y >= other.min.Y) &&
		(min.Z <= other.max.Z && max.Z >= other.min.Z);
}

bool AABB3D::contains(const vec3& point) const {
	return (point.X >= min.X && point.X <= max.X) &&
		(point.Y >= min.Y && point.Y <= max.Y) &&
		(point.Z >= min.Z && point.Z <= max.Z);
}

Collision2D::Collision2D() : Instance() {

	name = "AABBCollider2D";
	type = InstanceType_Collision2D;

	position = vec2(0.0f, 0.0f);
	size = vec2(1.0f, 1.0f);
	boundingBox = AABB2D(position, position + size);
}

bool Collision2D::checkCollision(Collision2D* other) const {
	return boundingBox.intersects(other->boundingBox);
}

void Collision2D::setPosition(const vec2& pos) {
	position = pos;
	boundingBox = AABB2D(position, position + size);
}

void Collision2D::setSize(const vec2& newSize) {
	size = newSize;
	boundingBox = AABB2D(position, position + size);
}

vec2 Collision2D::getPosition() const {
	return position;
}

vec2 Collision2D::getSize() const {
	return size;
}

vec2 Collision2D::getCenter() const {
	return (position) + size * 0.5f;
}

Collision3D::Collision3D() : SpatialInstance()
{
	name = "Collider";
	type = InstanceType_Collision3D;
	updateBoundingBox();
	preserveRotatedSize = false;
	wrapAroundParentMesh = false;
	alwaysUpdate = true;
}

bool Collision3D::checkCollision(Collision3D* other) const {
	//printf("Me - Min: (%f, %f, %f), Max: (%f, %f, %f)\n", boundingBox.min.X, boundingBox.min.Y, boundingBox.min.Z, boundingBox.max.X, boundingBox.max.Y, boundingBox.max.Z);
	//printf("Other - Min: (%f, %f, %f), Max: (%f, %f, %f)\n", other->boundingBox.min.X, other->boundingBox.min.Y, other->boundingBox.min.Z, other->boundingBox.max.X, other->boundingBox.max.Y, other->boundingBox.max.Z);
	return boundingBox.intersects(other->boundingBox);
}

void Collision3D::setPosition(const vec3& pos) {
	position = pos;
	updateBoundingBox();
}

void Collision3D::setSize(const vec3& newSize) {
	scale = newSize;
	updateBoundingBox();
}

vec3 Collision3D::getPosition() const {
	return boundingBox.min;
}

vec3 Collision3D::getSize() const {
	return boundingBox.max - boundingBox.min;
}

vec3 Collision3D::getCenter() const {
	return (boundingBox.min) + getSize() * 0.5f;
}
bool Collision2D::isInside(vec2 point) const {
	return boundingBox.contains(point);
}

bool Collision3D::isInside(vec3 point) const {
	return boundingBox.contains(point);
}

Instance* Collision2D::clone_this() {
	Collision2D* newCollider = (Collision2D*)registerInstance(new Collision2D(*this), true);
	newCollider->setPosition(this->getPosition());
	newCollider->setSize(this->getSize());
	return newCollider;
}

Instance* Collision3D::clone_this() {
	Collision3D* newCollider = (Collision3D*)registerInstance(new Collision3D(*this), true);
	newCollider->position = this->position;
	newCollider->scale = this->scale;
	newCollider->updateBoundingBox();
	newCollider->makeActive();
	return newCollider;
}

int Collision2D::saveInstanceToFile(FILE* file) {
	Instance::saveInstanceToFile(file);
	fwrite(&position, sizeof(vec2), 1, file);
	fwrite(&size, sizeof(vec2), 1, file);
	return 0; //TODO: return the size of the data written to the file
}

int Collision2D::loadInstanceFromFile(void* data, int off) {
	int o = off;
	o += Instance::loadInstanceFromFile(data, o); //Load the base instance data first
	position = *(vec2*)offset(data, o);
	o += sizeof(vec2);
	size = *(vec2*)offset(data, o);
	o += sizeof(vec2);
	boundingBox = AABB2D(position, position + size);
	return o - off; // Return the number of bytes read
}

int Collision3D::saveInstanceToFile(FILE* file) {
	SpatialInstance::saveInstanceToFile(file);
	fwrite(&preserveRotatedSize, sizeof(bool), 1, file);
	fwrite(&wrapAroundParentMesh, sizeof(bool), 1, file);
	fwrite(&alwaysUpdate, sizeof(bool), 1, file);
	return 0; //TODO: return the size of the data written to the file
}

int Collision3D::loadInstanceFromFile(void* data, int off) {
	int o = off;
	o += SpatialInstance::loadInstanceFromFile(data, o); //Load the base instance data first

	preserveRotatedSize = *(bool*)offset(data, o);
	o += sizeof(bool); //preserveRotatedSize
	wrapAroundParentMesh = *(bool*)offset(data, o);
	o += sizeof(bool); //wrapAroundParentMesh
	alwaysUpdate = *(bool*)offset(data, o);
	o += sizeof(bool); //wrapAroundParentMesh
	updateBoundingBox();
	return o - off; // Return the number of bytes read
}

void Collision3D::debugRender() {
	//GL_LINES
	Buffer vertexBuffer;

	vertexBuffer.data = new vec3[24]{
		//Back
		vec3(0, 0, 0),
		vec3(1, 0, 0),

		vec3(1, 0, 0),
		vec3(1, 1, 0),

		vec3(1, 1, 0),
		vec3(0, 1, 0),

		vec3(0, 1, 0),
		vec3(0, 0, 0),

		//Front
		vec3(0, 0, 1),
		vec3(1, 0, 1),

		vec3(1, 0, 1),
		vec3(1, 1, 1),

		vec3(1, 1, 1),
		vec3(0, 1, 1),

		vec3(0, 1, 1),
		vec3(0, 0, 1),

		//Cross
		vec3(0, 0, 0),
		vec3(0, 0, 1),

		vec3(1, 0, 0),
		vec3(1, 0, 1),

		vec3(1, 1, 0),
		vec3(1, 1, 1),

		vec3(0, 1, 0),
		vec3(0, 1, 1),
	};

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
	program->setUniformSMat(program->shaderUniformNames[eSUniform_CameraLookMatrix], camera->view, GL_TRUE, 3);
	program->setUniformSMat(program->shaderUniformNames[eSUniform_CameraProjectionMatrix], camera->projection, GL_TRUE, 4);
	program->setUniform1f(program->shaderUniformNames[eSUniform_CurrentTime], Graphics::getInstance()->lastFrameTime);
	program->setUniform3f(program->shaderUniformNames[eSUniform_Material_DiffuseColor], Graphics::debugDefaultColor);
	glBindVertexArray(vao);
	glDrawArrays(GL_LINES, 0, 24);
	glBindVertexArray(0);

	// Draw call 2 - raw effective (axis-aligned) bounding box (bounding box to the bounding box);
	mat4 rawTransform = mat4::transform(boundingBox.min, getSize(), vec3(0, 0, 0));
	program->setUniformSMat(program->shaderUniformNames[eSUniform_ModelTransformMatrix], rawTransform, GL_TRUE, 4);
	program->setUniform3f(program->shaderUniformNames[eSUniform_Material_DiffuseColor], vec3(0.0f, 1.0f, 1.0f));
	glBindVertexArray(vao);
	glDrawArrays(GL_LINES, 0, 24);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);

}

void Collision3D::RenderAllCollisionBoxesDebug() {
	for (int i : allCollisionBoxes) {
		//printf("Rendering %d\n", i);
		((Collision3D*)getInstanceById(i))->debugRender();
	}
}

int Collision3D::destroy() {
	allCollisionBoxes.erase(getId());
	return Instance::destroy();
}

int Collision3D::getNumSaveBytes() {
	return 3 * sizeof(bool) + SpatialInstance::getNumSaveBytes();
}


void Collision3D::updateBoundingBox() {

	/*
	TODO: Add a flag for whether the bounding box should be updated to a parent mesh 
	 - Would use it's parent position/transform and use the mesh's bounding box in place of the local 0-1 cube. 
	This would allow for a more accurate bounding box for meshes that are not unit cubes.
	*/
	vec3 wp = getWorldPosition();
	vec3 size = getWorldScale();
	mat4 transform = getWorldTransform();
	
	SpatialInstance* p = getNextSpatialParent();

	if (wrapAroundParentMesh && p) {
		if (!Instance::isInstanceOf(Instance::InstanceType_Mesh, p->type)) {
			// If the parent is not a mesh, we can't calculate the bounding box.

			if (preserveRotatedSize) {
				getBoundingBoxFromPosition();
			}
			else {
				getBoundingBoxFromCorners();
			}
			return;
		}
		Mesh* parentMesh = (Mesh*)p;
		Geometry* parentGeometry = (Geometry*)getInstanceById(parentMesh->geometry);
		if (!parentGeometry) {
			// If the parent mesh doesn't have a geometry, we can't calculate the bounding box.
			if (preserveRotatedSize) {
				getBoundingBoxFromPosition();
			}
			else {
				getBoundingBoxFromCorners();
			}
			return;
		}

		//Inherit transform entirely from the parent
		position = parentGeometry->boundingBoxMin; // Center the collider on the parent mesh's bounding box
		rotation = vec3(0, 0, 0);
		scale = parentGeometry->boundingBoxMax - parentGeometry->boundingBoxMin;

		/*vec3 corners[8] = {
			vec3(parentGeometry->boundingBoxMin.X, parentGeometry->boundingBoxMin.Y, parentGeometry->boundingBoxMin.Z),
			vec3(parentGeometry->boundingBoxMax.X, parentGeometry->boundingBoxMin.Y, parentGeometry->boundingBoxMin.Z),
			vec3(parentGeometry->boundingBoxMin.X, parentGeometry->boundingBoxMax.Y, parentGeometry->boundingBoxMin.Z),
			vec3(parentGeometry->boundingBoxMin.X, parentGeometry->boundingBoxMin.Y, parentGeometry->boundingBoxMax.Z),
			vec3(parentGeometry->boundingBoxMax.X, parentGeometry->boundingBoxMax.Y, parentGeometry->boundingBoxMin.Z),
			vec3(parentGeometry->boundingBoxMax.X, parentGeometry->boundingBoxMin.Y, parentGeometry->boundingBoxMax.Z),
			vec3(parentGeometry->boundingBoxMin.X, parentGeometry->boundingBoxMax.Y, parentGeometry->boundingBoxMax.Z),
			vec3(parentGeometry->boundingBoxMax.X, parentGeometry->boundingBoxMax.Y, parentGeometry->boundingBoxMax.Z)
		}; */


		if (preserveRotatedSize) {
			getBoundingBoxFromPosition();
		}
		else {
			getBoundingBoxFromCorners();
		}
	} else {
		if (preserveRotatedSize) {
			getBoundingBoxFromPosition();
		}
		else {
			getBoundingBoxFromCorners();
		}
	}
}

void Collision3D::getBoundingBoxFromPosition() {
	vec3 center = getWorldTransform() * vec3(0.5f, 0.5f, 0.5f);
	vec3 size = getWorldScale();

	size.X = std::abs(size.X);
	size.Y = std::abs(size.Y);
	size.Z = std::abs(size.Z);

	boundingBox = AABB3D(
		center - size * 0.5f,
		center + size * 0.5f
	);
}

void Collision3D::getBoundingBoxFromCorners(vec3* providedCorners) {

	mat4 transform = getWorldTransform();

	vec3 corners[8];
	if (!providedCorners) {
		corners[0] = transform * vec3(0, 0, 0);
		corners[1] = transform * vec3(1, 0, 0);
		corners[2] = transform * vec3(0, 1, 0);
		corners[3] = transform * vec3(0, 0, 1);
		corners[4] = transform * vec3(1, 1, 0);
		corners[5] = transform * vec3(1, 0, 1);
		corners[6] = transform * vec3(0, 1, 1);
		corners[7] = transform * vec3(1, 1, 1);
	}
	else {
		for (int i = 0; i < 8; ++i) {
			corners[i] = transform * providedCorners[i];
		}
	}
	vec3 minCorner = corners[0];
	vec3 maxCorner = corners[0];
	for (int i = 0; i < 8; ++i) {
		minCorner.X = std::min(minCorner.X, corners[i].X);
		minCorner.Y = std::min(minCorner.Y, corners[i].Y);
		minCorner.Z = std::min(minCorner.Z, corners[i].Z);
		maxCorner.X = std::max(maxCorner.X, corners[i].X);
		maxCorner.Y = std::max(maxCorner.Y, corners[i].Y);
		maxCorner.Z = std::max(maxCorner.Z, corners[i].Z);
	}
	boundingBox = AABB3D(minCorner, maxCorner);
}
void Collision3D::update(float deltaTime, bool isStudio) {
	SpatialInstance::update(deltaTime, isStudio);
	if (alwaysUpdate) {
		updateBoundingBox();
	}
}

void Collision3D::makeActive() {
	allCollisionBoxes.insert(getId());
	SpatialInstance::makeActive();
}

void Collision3D::makeInactive() {
	allCollisionBoxes.erase(getId());
	SpatialInstance::makeInactive();
}