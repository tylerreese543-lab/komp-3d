#pragma once
#include "GameInstance.h"
#include "Graphics/matrix.h"
#include "SpatialInstance.h"
class Camera : public SpatialInstance
{
public:

	mat4 projection;
	mat3 view;
	mat3 look;
	//Essentially the 2D direction of the camera, ignoring the Y axis. This is used for movement and other calculations.
	mat3 moveDirection2D;

	void updateMatrices();
	
	float near_plane;
	float far_plane;
	float fov;
	bool isOrthographic;
	float aspect_ratio;
	void setPosition(float x, float y, float z);
	void setPlanes(float near, float far);
	float fovGL();
	virtual void update(float deltaTime, bool isStudio);
	Camera();
	Camera(vec3);

	virtual Instance* clone_this();
	virtual int saveInstanceToFile(FILE* file);
	virtual int loadInstanceFromFile(void* data, int offset = 0);
	virtual int getNumSaveBytes();

	void debugRender();
};

