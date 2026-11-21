#pragma once
#include "GameInstance.h"
#include "Graphics/matrix.h"
class SpatialInstance : public Instance
{

protected:
	void updateTransformCache();
	bool isCacheDirty();
public:
	SpatialInstance();

	virtual void update(float deltaTime, bool isStudio);
	vec3 position;
	vec3 rotation;
	vec3 scale;

	vec3 cachedPosition;
	vec3 cachedRotation;
	vec3 cachedScale;

	mat4 cachedLocalTransform;


	mat4 getRawTransform();
	mat4 getLocalTransform();
	mat4 getWorldTransform();

	vec3 getWorldPosition();
	vec3 getWorldRotation();
	vec3 getWorldScale();

	//Recursively gets the next parent that is a SpatialInstance, or nullptr if there are no more parents.
	SpatialInstance* getNextSpatialParent();
	virtual Instance* clone_this();

	virtual int saveInstanceToFile(FILE* file);
	virtual int loadInstanceFromFile(void* data, int offset = 0);
	virtual int getNumSaveBytes();
};

