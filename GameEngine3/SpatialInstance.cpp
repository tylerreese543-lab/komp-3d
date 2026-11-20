#include "SpatialInstance.h"


SpatialInstance::SpatialInstance() : Instance() {
	position = vec3(0, 0, 0);
	rotation = vec3(0, 0, 0);
	scale = vec3(1, 1, 1);
	type = InstanceType_Spatial;
}

Instance* SpatialInstance::clone_this() {
	SpatialInstance* i = new SpatialInstance(*this);
	return (SpatialInstance*)registerInstance(i, true);
}

mat4 SpatialInstance::getLocalTransform() {
	mat4 translation = mat4::transform(position, scale, rotation);
	return mat4::transform(position, scale, rotation);
}

mat4 SpatialInstance::getWorldTransform() {
	if (parent == nullptr) {
		return getLocalTransform();
	}
	else {
		SpatialInstance* parentSpatial = getNextSpatialParent();
		if (parentSpatial) {
			return parentSpatial->getWorldTransform() * getLocalTransform();
		}
		else {
			return getLocalTransform();
		}
	}
}

vec3 SpatialInstance::getWorldPosition() {
	mat4 worldTransform = getWorldTransform();
	return vec3(worldTransform.m[0][3], worldTransform.m[1][3], worldTransform.m[2][3]);
}


/* TODO: Combine position, scale, and orientation into a single object (CoordinatePoint) to make operations easier + use quaternions */
vec3 SpatialInstance::getWorldRotation() {
	if (parent == nullptr) {
		return rotation;
	}
	else {
		SpatialInstance* parentSpatial = getNextSpatialParent();
		if (parentSpatial) {
			return parentSpatial->getWorldRotation() + rotation;
		}
		else {
			return rotation;
		}
	}
}

vec3 SpatialInstance::getWorldScale() {
	if (parent == nullptr) {
		return scale;
	}
	else {
		SpatialInstance* parentSpatial = getNextSpatialParent();
		if (parentSpatial) {
			vec3 parentScale = parentSpatial->getWorldScale();
			return vec3(scale.X * parentScale.X, scale.Y * parentScale.Y, scale.Z * parentScale.Z);
		}
		else {
			return scale;
		}
	}
}

SpatialInstance* SpatialInstance::getNextSpatialParent() {
	Instance* currentParent = parent;
	while (currentParent != nullptr) {
		SpatialInstance* spatialParent = dynamic_cast<SpatialInstance*>(currentParent);
		if (spatialParent && isInstanceOf(spatialParent->type, InstanceType_Spatial)) {
			return spatialParent;
		}
		currentParent = currentParent->parent;
	}

	return nullptr;
}

void SpatialInstance::update(float deltaTime, bool isStudio) {
	// Update logic for SpatialInstance can be added here if needed
}

int SpatialInstance::saveInstanceToFile(FILE* file) {
	Instance::saveInstanceToFile(file);
	fwrite(&position, sizeof(vec3), 1, file);
	fwrite(&rotation, sizeof(vec3), 1, file);
	fwrite(&scale, sizeof(vec3), 1, file);
	return 0;
}

int SpatialInstance::loadInstanceFromFile(void* data, int off) {
	int o = off;
	o += Instance::loadInstanceFromFile(data, o);
	position = *(vec3*)offset(data, o);
	o += sizeof(vec3);
	rotation = *(vec3*)offset(data, o);
	o += sizeof(vec3);
	scale = *(vec3*)offset(data, o);
	o += sizeof(vec3);
	return o - off;
}

int SpatialInstance::getNumSaveBytes() {
	return Instance::getNumSaveBytes() + sizeof(vec3) * 3;
}