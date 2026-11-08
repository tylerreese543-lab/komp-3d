#include "Material.h"



const std::string Material::containsMaterialTexture[] = {
	"m_HasDiffuseTexture",
	"m_HasAmbientOcclusionTexture",
	"m_HasNormalTexture",
	"m_HasSpecularTexture",
	"m_HasPositionTexture",
	"m_HasRoughnessTexture"
};
Material::Material() : Instance() {

	diffuseColor = vec4(1.0, 1.0, 1.0, 1.0);
	specularity = 32;
	roughness = 0.5;

	//Texture Resource Initialization
	textures[eMaterialTexture_Diffuse] = -1;
	textures[eMaterialTexture_AmbientOcclusion] = -1;
	textures[eMaterialTexture_Normal] = -1;
	textures[eMaterialTexture_Specular] = -1;
	textures[eMaterialTexture_Position] = -1;
	textures[eMaterialTexture_Roughness] = -1;
	shader = -1;
	frameBufferType = eFramebuffer_None;
	type = InstanceType_Material;
	name = "Material";
	isTransparent = false;
	isLit = true;
	SetTextureUseFlags(false, false, false, false);
}

void Material::SetTextureUseFlags(bool useDiffuse, bool useAmbientOcclusion, bool useNormal, bool useSpecular) {
	textureUseFlags[eMaterialTexture_Diffuse] = useDiffuse;
	textureUseFlags[eMaterialTexture_AmbientOcclusion] = useAmbientOcclusion;
	textureUseFlags[eMaterialTexture_Normal] = useNormal;
	textureUseFlags[eMaterialTexture_Specular] = useSpecular;
	textureUseFlags[eMaterialTexture_Position] = false;
	textureUseFlags[eMaterialTexture_Roughness] = false;
}

//Searches through all of it's children for viable textures.
void Material::autoPrepare(bool autoFlags) {
	for (int i = eMaterialTexture_Diffuse; i < eMaterialTexture_MAX; ++i) {
		if (textureUseFlags[i] == false && (!autoFlags)) continue;
		for (Instance* child : children) {
			ImageResource* img = isImage(child);

			if (!img) continue;
			if (!img->loaded) img->loadFromFile(img->filePath);
			


			textures[img->textureType] = img->getId();
			if (autoFlags) {
				textureUseFlags[img->textureType] = true;
			}

		}
	}
	for (Instance* child : children) {
		ShaderProgram* shdr = isShader(child);
		if (!shdr) continue;
		shader = shdr->getId();
		break;
	}
}

void Material::applyMaterial() {
	
	ShaderProgram* program = isShader(getInstanceById(shader));
	if (!program) return;
	
	program->useProgram();
	for (int i = 0; i < eMaterialTexture_MAX; ++i) {
		if (textureUseFlags[i] == true) {
			program->unbindTexture(program->shaderTextureNames[i], i);
			program->setUniform1i(containsMaterialTexture[i], 0);
			ImageResource* img = isImage(getInstanceById(textures[i]));
			if (!img) continue;
			int boundSuccessfully = program->bindTexture(program->shaderTextureNames[i], img, i);
			program->setUniform1i(containsMaterialTexture[i], boundSuccessfully == 0 ? 1 : 0);
		}
		else {
			program->setUniform1i(containsMaterialTexture[i], 0);
		}
	}

	program->setUniform4f(program->shaderUniformNames[eSUniform_Material_DiffuseColor], diffuseColor);
	program->setUniform1i(program->shaderUniformNames[eSUniform_Material_Specularity], specularity);
	program->setUniform1f(program->shaderUniformNames[eSUniform_Material_Roughness], roughness);
}
void Material::applyMaterialFramebuffer() {
	
	ShaderProgram* program = isShader(getInstanceById(shader));
	if (!program) return;
	
	Graphics::getInstance()->getFramebuffer(frameBufferType)->bind();
	program->useProgram();
	GLenum frameBufferAttachments[eMaterialTexture_MAX];
	for (int i = 0; i < eMaterialTexture_MAX; ++i) {
		if (textureUseFlags[i] == true) {
			program->unbindTexture(program->shaderTextureNames[i], i);
			ImageResource* img = isImage(getInstanceById(textures[i]));
			if (!img) {
			frameBufferAttachments[i] = GL_NONE;
			continue;
			};
			int boundSuccessfully = program->bindTexture(program->shaderTextureNames[i], img, i);
			if (boundSuccessfully == 0) {
				frameBufferAttachments[i] = (GL_COLOR_ATTACHMENT0 + img->textureType);
			}
			else {
				frameBufferAttachments[i] = GL_NONE;
			}
			program->unbindTexture(program->shaderTextureNames[i], i);
		}
		else {
			frameBufferAttachments[i] = GL_NONE;
		}
	}

	glDrawBuffers(eMaterialTexture_MAX, frameBufferAttachments);
	Graphics::getInstance()->getFramebuffer(frameBufferType)->unbind();
	//program->setUniform3f(program->shaderUniformNames[eSUniform_Material_DiffuseColor], diffuseColor.X, diffuseColor.Y, diffuseColor.Z);
	//program->setUniform1f(program->shaderUniformNames[eSUniform_Material_Specularity], specularity);
}
ImageResource* Material::isImage(Instance* image) {
	if (!image) return nullptr;
	if (image->type != InstanceType_ImageResource) return nullptr; //Skip any non-resources
	if (!isRegistered(image)) return nullptr;
	ImageResource* img = dynamic_cast<ImageResource*>(image);
	if (img->textureType < 0 || img->textureType > eMaterialTexture_MAX) return nullptr; // -1 = not a texture resource, don't use this
	return img;
}

ShaderProgram* Material::isShader(Instance* shdr) {
	if (!shdr) return nullptr;
	if (shdr->type != InstanceType_ShaderProgram) return nullptr; //Skip any non-resources
	if (!isRegistered(shdr)) return nullptr;
	ShaderProgram* shader = dynamic_cast<ShaderProgram*>(shdr);
	return shader;
}

void Material::setTexture(ImageResource* i) {
	i = isImage(i);
	if (!i) return;
	textures[i->textureType] = i->getId();
}
void Material::setTextureById(int id, int target) {
	ImageResource* img = isImage(getInstanceById(id));

	if (id == -1 or img) {
		if (img) target = img->textureType;
		textures[target] = id;
	}
}
void Material::setShader(ShaderProgram* i) {
	i = isShader(i);
	if (!i) {
		shader = -1;
	}
	else {
		shader = i->getId();
	}
}

int Material::getShaderInstanceId() {
	return shader;
}
int Material::getTextureInstanceId(int target) {
	if (target < 0 || target > eMaterialTexture_MAX) return -1;
	return textures[target];
}
Instance* Material::clone_this() {
	return registerInstance(new Material(*this), true);
}

int Material::saveInstanceToFile(FILE* file) {
	Instance::saveInstanceToFile(file);
	fwrite(&diffuseColor, sizeof(vec4), 1, file);
	fwrite(&specularity, sizeof(int), 1, file);
	fwrite(&roughness, sizeof(int), 1, file);
	fwrite(&shader, sizeof(int), 1, file);
	fwrite(textures, sizeof(int), eMaterialTexture_MAX, file);
	fwrite(textureUseFlags, sizeof(bool), eMaterialTexture_MAX, file);
	fwrite(&isTransparent, sizeof(bool), 1, file);
	fwrite(&isLit, sizeof(bool), 1, file);
	return 0; //TODO: return the size of the data written to the file
}

int Material::loadInstanceFromFile(void* data, int off) {
	int o = off;
	o += Instance::loadInstanceFromFile(data, o); //Load the base instance data first
	diffuseColor = *(vec4*)offset(data, o);
	o += sizeof(vec4);
	specularity = *(int*)offset(data, o);
	o += sizeof(int);
	roughness = *(float*)offset(data, o);
	o += sizeof(float);
	shader = *(int*)offset(data, o);
	o += sizeof(int);
	memcpy(textures, offset(data, o), sizeof(int) * eMaterialTexture_MAX);
	o += sizeof(int) * eMaterialTexture_MAX;
	memcpy(textureUseFlags, offset(data, o), sizeof(bool) * eMaterialTexture_MAX);
	o += sizeof(bool) * eMaterialTexture_MAX;
	isTransparent = *(bool*)offset(data, o);
	o += sizeof(bool);
	isLit = *(bool*)offset(data, o);
	o += sizeof(bool);
	return o - off; // Return the number of bytes read
}

int Material::getNumSaveBytes() {
	return Instance::getNumSaveBytes() + sizeof(vec4) + sizeof(float) + (sizeof(int) * 2) + (sizeof(int) * eMaterialTexture_MAX) + (sizeof(bool) * eMaterialTexture_MAX) + (2 * sizeof(bool));
}

void Material::finalizeLoad(inst_conv_map* conversionMap) {
	Instance::finalizeLoad(conversionMap);
	if (conversionMap->count(shader) > 0) {
		shader = (*conversionMap)[shader];
	}
	for (int i = 0; i < eMaterialTexture_MAX; ++i) {
		if (conversionMap->count(textures[i]) > 0) {
			textures[i] = (*conversionMap)[textures[i]];
		}
	}
}