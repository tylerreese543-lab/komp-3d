#pragma once
#include "GameInstance.h"
#include "TextureResource.h"
#include "Graphics/GraphicsWrapper.h"
#include "Graphics/matrix.h"
#include "Shader.h"
class Material : public Instance
{
	ImageResource* isImage(Instance* inst);
	ShaderProgram* isShader(Instance* inst);

	const static std::string containsMaterialTexture[eMaterialTexture_MAX];
	int textures[eMaterialTexture_MAX];
	int shader;
public:
	//Can be from another instance, independent, or it's own children

	bool textureUseFlags[eMaterialTexture_MAX];

	vec4 diffuseColor;
	int specularity;
	float roughness;
	//Whether or not this material will shade objects non-opaquely.
	bool isTransparent;
	//Whether or not this material will follow the fixed lighting pipeline.
	bool isLit;
	int frameBufferType;
	Material();

	void setTexture(ImageResource* index);
	void setShader(ShaderProgram* index);
	void setTextureById(int id, int target);
	//Will automatically look for child textures if a flag is present but no texture is bound.
	void autoPrepare(bool autoFlags);
	void SetTextureUseFlags(bool useDiffuse, bool useAmbientOcclusion, bool useNormal, bool useSpecular);
	void applyMaterial();
	void applyMaterialFramebuffer();

	int getShaderInstanceId();

	ShaderProgram* getShader();
	int getTextureInstanceId(int target);
	virtual Instance* clone_this();

	virtual int saveInstanceToFile(FILE* file);
	virtual int loadInstanceFromFile(void* data, int off = 0);

	virtual int getNumSaveBytes();

	virtual void finalizeLoad(std::map<int, int>* conversionMap);
};

