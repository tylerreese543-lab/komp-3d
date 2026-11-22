#include "Shader.h"

#include <string>
#include <string_view>
#include <utility>
#include <cctype>


#include "Light.h"
const std::string ShaderResource::coreShaderPortNames[2] = {
	"vertex_main",
	"fragment_main"
};
const std::string ShaderResource::coreShaderSourceInjectionPoint = "// --- [INJECT USER CODE HERE] ---";
const std::string ShaderResource::coreShaderSourcePath[] = {
	"core://shader/core_vertex_wrapper.glsl",
	"core://shader/core_fragment_wrapper.glsl"
};

std::string ShaderResource::coreShaderSource[][2] = {
	{"", ""}, //Vertex Shader
	{"", ""}  //Fragment Shader
};

void ShaderResource::LoadCoreShaderSources() {
	for (int i = 0; i < 2; i++) {
		FILE* file = openResourceFile(coreShaderSourcePath[i]);
		if (!file) {
			printf("ERROR: Failed to load core shader source from '%s'.\n", coreShaderSourcePath[i].c_str());
			continue;
		}
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char* buffer = new char[size + 1];
		fread(buffer, sizeof(char), size, file);
		buffer[size] = '\0';

		std::string shaderSource;
		shaderSource = buffer;

		std::string header = shaderSource.substr(0, shaderSource.find(coreShaderSourceInjectionPoint));
		std::string footer = shaderSource.substr(shaderSource.find(coreShaderSourceInjectionPoint) + coreShaderSourceInjectionPoint.length());

		coreShaderSource[i][0] = header;
		coreShaderSource[i][1] = footer;
		delete[] buffer;
		fclose(file);
	}
}

void ShaderProgram::update(float deltaTime, bool isStudio) {

	useProgram();
	setUniform3f("l_AmbientLight", Light::ambient);
	setUniform1i("l_NumPointLights", Light::numPointLights);
	setUniform1i("l_NumDirectionalLights", Light::numDirectionalLights);
	setUniform1f(shaderUniformNames[eSUniform_CurrentTime], Graphics::getInstance()->lastFrameTime);
}
std::string ShaderResource::getFullShaderCodeFromSource(std::string* source, GLenum t) {
	std::string sourceConcat[3] = {"", "", ""};
	sourceConcat[1] = getShaderCodeFromSource(source, t);

	if (t == GL_VERTEX_SHADER) {
		sourceConcat[0] = coreShaderSource[0][0];
		sourceConcat[2] = coreShaderSource[0][1];
	} else if (t == GL_FRAGMENT_SHADER) {
		sourceConcat[0] = coreShaderSource[1][0];
		sourceConcat[2] = coreShaderSource[1][1];
	}
	return sourceConcat[0] + sourceConcat[1] + sourceConcat[2];
}
std::string ShaderResource::getFullShaderSourceCode() {
	return getFullShaderCodeFromSource(&source, shaderType);
}
std::string ShaderResource::getShaderCodeFromSource(std::string* source, GLenum t) {
    // Plain integer states instead of an enum: 0 = Code, 1 = LineComment, 2 = BlockComment
    auto findFunctionSpan = [](std::string_view code, std::string_view funcName) -> std::pair<size_t, size_t> {
        int state = 0;
        size_t funcPos = std::string_view::npos;

        // 1. Scan for the standalone function name while skipping comments
        for (size_t i = 0; i < code.size(); ++i) {
            if (state == 0) {
                if (code.substr(i, 2) == "//") {
                    state = 1;
                    i++;
                } else if (code.substr(i, 2) == "/*") {
                    state = 2;
                    i++;
                } else if (code.substr(i, funcName.size()) == funcName) {
                    bool leftClear = (i == 0) || (!std::isalnum(static_cast<unsigned char>(code[i - 1])) && code[i - 1] != '_');
                    bool rightClear = (i + funcName.size() >= code.size()) || (!std::isalnum(static_cast<unsigned char>(code[i + funcName.size()])) && code[i + funcName.size()] != '_');
                    
                    if (leftClear && rightClear) {
                        funcPos = i;
                        break;
                    }
                }
            } else if (state == 1) {
                if (code[i] == '\n') state = 0;
            } else if (state == 2) {
                if (code.substr(i, 2) == "*/") {
                    state = 0;
                    i++;
                }
            }
        }

        if (funcPos == std::string_view::npos) {
            return { std::string::npos, std::string::npos };
        }

        // 2. Walk backwards to the start of the line (or end of a preceding block comment)
        size_t start = 0;
        for (size_t i = funcPos; i > 0; --i) {
            if (code[i - 1] == '\n') {
                start = i;
                break;
            }
            if (i >= 2 && code[i - 2] == '*' && code[i - 1] == '/') {
                start = i;
                break;
            }
        }

        // 3. Scan forward from funcPos to count braces, ignoring comments
        int depth = 0;
        bool foundOpenBrace = false;
        state = 0;

        for (size_t i = funcPos + funcName.size(); i < code.size(); ++i) {
            if (state == 0) {
                if (code.substr(i, 2) == "//") {
                    state = 1;
                    i++;
                } else if (code.substr(i, 2) == "/*") {
                    state = 2;
                    i++;
                } else if (code[i] == '{') {
                    depth++;
                    foundOpenBrace = true;
                } else if (code[i] == '}') {
                    depth--;
                    if (foundOpenBrace && depth == 0) {
                        return { start, i + 1 };
                    }
                }
            } else if (state == 1) {
                if (code[i] == '\n') state = 0;
            } else if (state == 2) {
                if (code.substr(i, 2) == "*/") {
                    state = 0;
                    i++;
                }
            }
        }

        return { std::string::npos, std::string::npos };
    };

    auto maskOutSpan = [](std::string_view code, std::pair<size_t, size_t> span) -> std::string {
        std::string result(code);
        if (span.first == std::string::npos || span.second == std::string::npos) {
            return result;
        }

        for (size_t i = span.first; i < span.second; ++i) {
            if (result[i] != '\n') {
                result[i] = ' ';
            }
        }
        return result;
    };


    // Vertex gets everything except fragment_main; Fragment gets everything except vertex_main
    
	if (t == GL_VERTEX_SHADER) {
		return maskOutSpan(*source, findFunctionSpan(*source, coreShaderPortNames[1]));
	} else if (t == GL_FRAGMENT_SHADER) {
		return maskOutSpan(*source, findFunctionSpan(*source, coreShaderPortNames[0]));
	} else {
		return *source; // Fallback for unknown shader types
	}
}

std::string ShaderResource::getShaderSourceCode() {
	return getShaderCodeFromSource(&source, shaderType);
}
const std::string ShaderResource::shaderTypeNames[] = {
	"Vertex Shader",
	"Fragment Shader"
};
const GLenum ShaderResource::shaderTypeEnums[] = {
	GL_VERTEX_SHADER,
	GL_FRAGMENT_SHADER
};
ShaderResource::ShaderResource() : Resource() {
	name = "Shader";
	type = InstanceType_ShaderResource;
	id = 0;
	compiled = false;
	compileResult = 0;
	source = "";
	shaderType = GL_VERTEX_SHADER;

}

ShaderResource::ShaderResource(GLenum type) : ShaderResource() {
	this->shaderType = type;
}
ShaderResource::ShaderResource(GLenum type, const char* source) : ShaderResource() {
	this->shaderType = type;
	this->source = (char*)source;
}

int ShaderResource::loadFromFile(const char* filepath) {
	compiled = false;
	filePath = filepath;
	FILE* file = openResourceFile(filepath);
	if (!file) return -1;
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* buffer = new char[size + 1];
	fread(buffer, sizeof(char), size, file);
	buffer[size] = '\0';
	source = buffer;
	delete[] buffer;
	fclose(file);
	return 0;
}


int ShaderResource::compileShader() {
	compiled = true;

	if (id != 0) {
		glDeleteShader(id);
		id = 0;
	}
	id = glCreateShader(shaderType);
	if (source.empty()) {
		printf("ERROR: No source code provided for shader '%s'.\n", filePath.c_str());
		return -1;
	}

	if (isEditorIntrinsic) {
		const char* source_cstr = source.c_str();
		glShaderSource(id, 1, &source_cstr, nullptr);
	}
	else {
		std::string shaderConcat = getFullShaderSourceCode();
		const char* shaderConcat_cstr = shaderConcat.c_str();
		glShaderSource(id, 1, &shaderConcat_cstr, nullptr);
	}
	glCompileShader(id);
	glGetShaderiv(id, GL_COMPILE_STATUS, &compileResult);
	if (compileResult != GL_TRUE) {
		int infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* message = new char[infoLogLength];
		glGetShaderInfoLog(id, infoLogLength, &infoLogLength, message);
		printf("ERROR: Failed to compile shader '%s': %s\n", filePath.c_str(), message);
		delete[] message;
		return -1;
	}
	return 0;
}

bool ShaderResource::isCompiled() {
	return compiled;
}

bool ShaderResource::isValid() {
	return compiled && (compileResult == GL_TRUE);
}

ShaderProgram::ShaderProgram() : Resource() {
	name = "ShaderProgram";
	type = InstanceType_ShaderProgram;
	id = 0;
	loadInterleaved = false;
	
	setDefaultVertexAttribute(eVAttribute_Position,					"m_Position");
	setDefaultVertexAttribute(eVAttribute_UV,						"m_TexCoord");
	setDefaultVertexAttribute(eVAttribute_Normal,					"m_Normal");
	setDefaultVertexAttribute(eVAttribute_Color,					"m_Color");

	setDefaultShaderUniform(eSUniform_CameraLookMatrix,				"c_LookMatrix");
	setDefaultShaderUniform(eSUniform_CameraPosition,				"c_Position");
	setDefaultShaderUniform(eSUniform_ModelTransformMatrix,			"m_Transform");
	setDefaultShaderUniform(eSUniform_CameraProjectionMatrix,		"c_ProjectionMatrix");
	setDefaultShaderUniform(eSUniform_Material_DiffuseColor,		"m_DiffuseColor");
	setDefaultShaderUniform(eSUniform_Material_Specularity,			"m_Specularity");
	setDefaultShaderUniform(eSUniform_CurrentTime,					"u_CurrentTime");
	setDefaultShaderUniform(eSUniform_Text_Font,					"tx_TextGlyphs");
	setDefaultShaderUniform(eSUniform_Material_Roughness,			"m_Roughness");

	setDefaultMaterialTexture(eMaterialTexture_Diffuse,				"t_Diffuse");
	setDefaultMaterialTexture(eMaterialTexture_AmbientOcclusion,	"t_AmbientOcclusion");
	setDefaultMaterialTexture(eMaterialTexture_Normal,				"t_Normal");
	setDefaultMaterialTexture(eMaterialTexture_Specular,			"t_Specular");
	setDefaultMaterialTexture(eMaterialTexture_Position,			"t_Position");
	setDefaultMaterialTexture(eMaterialTexture_Roughness,			"t_Roughness");

	setDefaultOutputPort(eMaterialTexture_Diffuse,					"g_Diffuse");
	setDefaultOutputPort(eMaterialTexture_AmbientOcclusion,			"g_AmbientOcclusion");
	setDefaultOutputPort(eMaterialTexture_Normal,					"g_Normal");
	setDefaultOutputPort(eMaterialTexture_Specular,					"g_Specular");
	setDefaultOutputPort(eMaterialTexture_Position,					"g_Position");
	setDefaultOutputPort(eMaterialTexture_Roughness,				"g_Roughness");
}

ShaderProgram::ShaderProgram(std::string vs, std::string fs) : ShaderProgram() {
	ShaderResource* vertex_shader = new ShaderResource(GL_VERTEX_SHADER);
	vertex_shader->loadFromFile(vs.c_str());
	vertex_shader->name = "Vertex Shader";
	ShaderResource* fragment_shader = new ShaderResource(GL_FRAGMENT_SHADER);
	fragment_shader->loadFromFile(fs.c_str());
	fragment_shader->name = "Fragment Shader";
	addChild(vertex_shader);
	addChild(fragment_shader);
}
ShaderProgram::~ShaderProgram() {
	glDeleteProgram(id);
}

int ShaderProgram::attachShader(ShaderResource* shader) {
	glAttachShader(id, shader->id);
	return 0;
}

int ShaderProgram::linkProgram() {
	glLinkProgram(id);
	GLint linkResult;
	glGetProgramiv(id, GL_LINK_STATUS, &linkResult);
	if (linkResult != GL_TRUE) {
		int infoLogLength;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* message = new char[infoLogLength];
		glGetProgramInfoLog(id, infoLogLength, &infoLogLength, message);
		printf("ERROR: Failed to link shader program [ID %d] '%s': %s\n",id, name.c_str(), message);
		delete[] message;
		return -1;
	}

	GLuint lightBlockIndex = glGetUniformBlockIndex(this->id, "PointLightData");
    if (lightBlockIndex != GL_INVALID_INDEX) {
        glUniformBlockBinding(this->id, lightBlockIndex, 1);
    }

	lightBlockIndex = glGetUniformBlockIndex(this->id, "DirectionalLightData");
    if (lightBlockIndex != GL_INVALID_INDEX) {
        glUniformBlockBinding(this->id, lightBlockIndex, 2);
    }

	return 0;
}

std::string ShaderProgram::loadSourceFromFile() {
	FILE* file = openResourceFile(filePath);
	if (!file) return "";
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* buffer = new char[size + 1];
	fread(buffer, sizeof(char), size, file);
	buffer[size] = '\0';
	std::string source = buffer;
	delete[] buffer;
	fclose(file);
	return source;
}

int ShaderProgram::compileShaderStandalone(GLenum t, std::string* source) {

	int shaderId = 0;

	shaderId = glCreateShader(t);


	std::string src = source ? *source : loadSourceFromFile();

	int compileResult = 0;
	if (src.empty()) {
		printf("ERROR: No source code provided for shader '%s'.\n", filePath.c_str());
		return -1;
	}

	if (isEditorIntrinsic) {
		const char* source_cstr = src.c_str();
		glShaderSource(shaderId, 1, &source_cstr, nullptr);
	}
	else {
		std::string shaderConcat = ShaderResource::getFullShaderCodeFromSource(&src, t);
		const char* shaderConcat_cstr = shaderConcat.c_str();
		glShaderSource(shaderId, 1, &shaderConcat_cstr, nullptr);
	}
	glCompileShader(shaderId);
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileResult);
	if (compileResult != GL_TRUE) {
		int infoLogLength;
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* message = new char[infoLogLength];
		glGetShaderInfoLog(shaderId, infoLogLength, &infoLogLength, message);
		printf("ERROR: Failed to compile shader '%s': %s\n", filePath.c_str(), message);
		delete[] message;
		return -1;
	}
	return shaderId;
}
int ShaderProgram::createProgramStandalone() {

	int shaderVertexId = compileShaderStandalone(GL_VERTEX_SHADER);
	int shaderFragmentId = compileShaderStandalone(GL_FRAGMENT_SHADER);

	glAttachShader(id, shaderVertexId);
	glAttachShader(id, shaderFragmentId);

	for (int i = 0; i < eMaterialTexture_MAX; ++i) {
		glBindFragDataLocation(id, i, shaderOutputNames[i].c_str());
	}
	linkProgram();
	glDeleteShader(shaderVertexId);
	glDeleteShader(shaderFragmentId);
	
	return 0;
}
int ShaderProgram::createProgram(bool reloadShaders) {

	if (id != 0) {
		glDeleteProgram(id);
		id = 0;
	}
	id = glCreateProgram();
	if (!isEditorIntrinsic) {
		//If this is not an editor intrinsic, then we need to load the core shader sources first
		createProgramStandalone();
	}
	else
	{

	std::vector<ShaderResource*> shaders;
	for (Instance* child : children) {
		if (child->type != InstanceType_ShaderResource) {
			continue;
		}
		shaders.push_back(dynamic_cast<ShaderResource*>(child));
	}

	for (ShaderResource* shader : shaders) {
		shader->isEditorIntrinsic = isEditorIntrinsic;
		if (reloadShaders) {
			shader->loadFromFile(shader->filePath.c_str());
		}
		shader->compileShader();
		attachShader(shader);
	}
	for (int i = 0; i < eMaterialTexture_MAX; ++i) {
		glBindFragDataLocation(id, i, shaderOutputNames[i].c_str());
	}
	linkProgram();
	for (ShaderResource* shader : shaders) {
		glDeleteShader(shader->id);
		shader->id = 0;
	}
}
	GLint numActiveAttribs = 0;
	glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &numActiveAttribs);

// Query the maximum name length to allocate an appropriately sized buffer
	GLint maxNameLength = 0;
	glGetProgramiv(id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);
// Create a buffer for the attribute names
	char* nameBuffer = new char[maxNameLength + 1];
	layout.clear();
	layout.elements.assign(numActiveAttribs, BufferLayoutElement());
	for (GLint i = 0; i < numActiveAttribs; ++i) {
		GLsizei actualLength = 0;
		GLint size = 0;
		GLenum type = 0;
		int count = 1;
    // Fetch attribute metadata (type, size, and text name)
		glGetActiveAttrib(id, i, maxNameLength, &actualLength, &size, &type, nameBuffer);
    // Fetch the actual layout/location index bound to this attribute
		GLint location = glGetAttribLocation(id, nameBuffer);
		if (type == GL_FLOAT_VEC4) {
			type = GL_FLOAT;
			count = 4;
		}
		if (type == GL_FLOAT_VEC3) {
			type = GL_FLOAT;
			count = 3;
		}
		if (type == GL_FLOAT_VEC2) {
			type = GL_FLOAT;
			count = 2;
		}
		if (location >= 0) {
			layout.elements[location] = BufferLayoutElement(type, count);
			if (loadInterleaved) {
				layout.elements[location].binding = eVBO_Positions;
			}
			else {
				//Set bindings to hard-coded indexes in the Mesh's VBO;
				if (strcmp(nameBuffer, shaderAttributeNames[eVAttribute_Position].c_str()) == 0) {
					layout.elements[location].binding = eVBO_Positions;
				}
				else if (strcmp(nameBuffer, shaderAttributeNames[eVAttribute_UV].c_str()) == 0) {
					layout.elements[location].binding = eVBO_UV;
				}
				else if (strcmp(nameBuffer, shaderAttributeNames[eVAttribute_Normal].c_str()) == 0) {
					layout.elements[location].binding = eVBO_Normal;
				}
				else if (strcmp(nameBuffer,  shaderAttributeNames[eVAttribute_Color].c_str()) == 0) {
					layout.elements[location].binding = eVBO_Color;
				}
				else {
					layout.elements[location].binding = eVBO_Extra;
				}
			}
			//printf("Element '%s' bound to VBO %d at location %d.\n", nameBuffer, layout.elements[location].binding, location);
		}
	
		else {
			printf("WARNING: attribute '%s' has a binding of %d.\n", nameBuffer, location);
		}
	}
	delete[] nameBuffer;
	return 0;
}

int ShaderProgram::useProgram() {
	glUseProgram(id);
	return 0;
}

int ShaderProgram::U(std::string uniform) {
	if (glGetUniformLocation(id, uniform.c_str()) == -1) {
		//Removed - developer allowed to intentionally exclude uniforms from shaders
		//printf("Uniform '%s' does not exist, or is not used in the shader program.\n", uniform.c_str());
	}
	return glGetUniformLocation(id, uniform.c_str());
}
void ShaderProgram::setUniform1i(std::string uniform, int f0) {
	glUniform1i(U(uniform), f0);
}
void ShaderProgram::setUniform1f(std::string uniform, float f0) {
	glUniform1f(U(uniform), f0);
}
void ShaderProgram::setUniform2f(std::string uniform, float f0, float f1) {
	glUniform2f(U(uniform), f0, f1);
}
void ShaderProgram::setUniform3f(std::string uniform, float f0, float f1, float f2) {
	glUniform3f(U(uniform), f0, f1, f2);
}
void ShaderProgram::setUniform4f(std::string uniform, float f0, float f1, float f2, float f3) {
	glUniform4f(U(uniform), f0, f1, f2, f3);
}
void ShaderProgram::setUniform2f(std::string uniform, vec2 v) {
	glUniform2f(U(uniform), v.X, v.Y);
}
void ShaderProgram::setUniform3f(std::string uniform, vec3 v) {
	glUniform3f(U(uniform), v.X, v.Y, v.Z);
}
void ShaderProgram::setUniform4f(std::string uniform, vec4 v) {
	glUniform4f(U(uniform), v.X, v.Y, v.Z, v.W);
}
void ShaderProgram::setUniformSMat(std::string uniform, const float* fv, bool transposed, int size) {
	switch (size) {
	case 4:
		glUniformMatrix4fv(U(uniform), 1, transposed, fv);
		break;
	case 3:
		glUniformMatrix3fv(U(uniform), 1, transposed, fv);
		break;
	case 2:
		glUniformMatrix2fv(U(uniform), 1, transposed, fv);
	default:
		printf("Not a valid square matrix size.\n");
		break;
	}
}

void ShaderProgram::setCameraUniforms(Camera* camera) {
	if (!camera) return;
	setUniform3f(shaderUniformNames[eSUniform_CameraPosition], camera->getWorldPosition());
	setUniformSMat(shaderUniformNames[eSUniform_CameraLookMatrix], camera->look, GL_TRUE, 3);
	setUniformSMat(shaderUniformNames[eSUniform_CameraProjectionMatrix], camera->projection, GL_TRUE, 4);
}
int ShaderProgram::bindTexture(std::string uniform, ImageResource* tex, int type) {
	int textureSlot = -1;

	if (U(uniform) == -1) return -1;
	//printf("%s -%d\n", uniform.c_str(), U(uniform));
	textureSlot = type;
	if (tex && textureSlot != -1) {
		if (tex->textureType == type || type == -1) {
			tex->BindSlot(textureSlot);
			glUniform1i(U(uniform), textureSlot);
			return 0;
		}
	}
	return -1;
}
void ShaderProgram::unbindTexture(std::string uniform, int type) {
	int textureSlot = -1;
	if (U(uniform) == -1) return;
	glGetUniformiv(id, U(uniform), &textureSlot);
	glActiveTexture(GL_TEXTURE0 + type);
	glBindTexture(GL_TEXTURE_2D, 0);
}
void ShaderProgram::unbindTextureArray(std::string uniform, int type) {
	int textureSlot = -1;
	if (U(uniform) == -1) return;
	glGetUniformiv(id, U(uniform), &textureSlot);
	glActiveTexture(GL_TEXTURE0 + type);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
void ShaderProgram::setDefaultVertexAttribute(int index, std::string name) {
	if (index >= 0 && index < eVAttribute_MAX) {
		shaderAttributeNames[index] = name;
	}
}

void ShaderProgram::setDefaultShaderUniform(int index, std::string name) {
	if (index >= 0 && index < eSUniform_MAX) {
		shaderUniformNames[index] = name;
	}
}

void ShaderProgram::setDefaultMaterialTexture(int index, std::string name) {
	if (index >= 0 && index < eMaterialTexture_MAX) {
		shaderTextureNames[index] = name;
	}
}

void ShaderProgram::setDefaultOutputPort(int index, std::string name) {
	if (index >= 0 && index < eMaterialTexture_MAX) {
		shaderOutputNames[index] = name;
	}
}

Instance* ShaderResource::clone_this() {
	ShaderResource* s = (ShaderResource*)registerInstance(new ShaderResource(*this), true);
	s->id = 0;
	s->compiled = false;
	s->compileResult = 0;
	s->filePath = filePath;
	if (!source.empty()) {
		size_t sourceLength = source.length();
		s->source = source;
	}
	else {
		s->source = "";
	}
	return s;
}

ShaderResource::~ShaderResource() {
	if (!source.empty()) {
		source.clear();
	}
	if (id != 0) {
		glDeleteShader(id);
		id = 0;
	}
}

int ShaderResource::saveInstanceToFile(FILE* file) {
	int offset = 0;
	offset += Resource::saveInstanceToFile(file);
	offset += fwrite(&shaderType, sizeof(GLenum), 1, file);
	offset += fwrite(filePath.data(), filePath.size() + 1, 1, file);
	offset += fwrite(&compileResult, sizeof(int), 1, file);
	offset += fwrite(&compiled, sizeof(bool), 1, file);
	if (!source.empty()) {
		size_t sourceLength = source.length();
		offset += fwrite(source.data(), sizeof(char), sourceLength + 1, file);
	}
	else {
		char emptySource = '\0';
		offset += fwrite(&emptySource, sizeof(char), 1, file);
	}
	return offset;
}

int ShaderResource::loadInstanceFromFile(void* data, int off) {
	int o = off;
	o += Resource::loadInstanceFromFile(data, o);
	memcpy(&shaderType, offset(data, o), sizeof(GLenum));
	o += sizeof(GLenum);
	char filePathBuffer[256];
	strcpy_s(filePathBuffer, sizeof(filePathBuffer), (char*)offset(data, o));
	filePath = std::string(filePathBuffer);
	o += strlen(filePathBuffer) + 1;
	memcpy(&compileResult, offset(data, o), sizeof(int));
	o += sizeof(int);
	memcpy(&compiled, offset(data, o), sizeof(bool));
	o += sizeof(bool);
	if (!source.empty()) {
		source.clear();
	}
	size_t sourceLength = strlen((char*)offset(data, o));
	if (sourceLength > 0) {
		source = std::string((char*)offset(data, o));
	}
	else {
		source = "";
	}
	o += sourceLength + 1;
	compiled = false;
	loadFromFile(filePath.c_str());
	return o - off;
}

int ShaderProgram::saveInstanceToFile(FILE* file) {
	int offset = 0;
	offset += Resource::saveInstanceToFile(file);
	offset += fwrite(&loadInterleaved, sizeof(bool), 1, file);
	for(int i = 0; i < eVAttribute_MAX; ++i) {
		offset += fwrite(shaderAttributeNames[i].data(), shaderAttributeNames[i].size() + 1, 1, file);
	}
	for(int i = 0; i < eSUniform_MAX; ++i) {
		offset += fwrite(shaderUniformNames[i].data(), shaderUniformNames[i].size() + 1, 1, file);
	}
	for(int i = 0; i < eMaterialTexture_MAX; ++i) {
		offset += fwrite(shaderTextureNames[i].data(), shaderTextureNames[i].size() + 1, 1, file);
	}
	for(int i = 0; i < eMaterialTexture_MAX; ++i) {
		offset += fwrite(shaderOutputNames[i].data(), shaderOutputNames[i].size() + 1, 1, file);
	}
	return offset;
}

int ShaderProgram::loadInstanceFromFile(void* data, int off) {
	int o = off;
	o += Resource::loadInstanceFromFile(data, o);
	loadInterleaved = *(bool*)offset(data, o);
	o += sizeof(bool);
	for (int i = 0; i < eVAttribute_MAX; ++i) {
		shaderAttributeNames[i] = std::string((char*)offset(data, o));
		o += shaderAttributeNames[i].size() + 1;
	}
	for (int i = 0; i < eSUniform_MAX; ++i) {
		shaderUniformNames[i] = std::string((char*)offset(data, o));
		o += shaderUniformNames[i].size() + 1;
	}
	for (int i = 0; i < eMaterialTexture_MAX; ++i) {
		shaderTextureNames[i] = std::string((char*)offset(data, o));
		o += shaderTextureNames[i].size() + 1;
	}
	for (int i = 0; i < eMaterialTexture_MAX; ++i) {
		shaderOutputNames[i] = std::string((char*)offset(data, o));
		o += shaderOutputNames[i].size() + 1;
	}
	debug_log("ShaderProgram loaded from file: %s\n", name.c_str());
	return o - off;
}

int ShaderResource::getNumSaveBytes() {
	int size = 0;
	size += Resource::getNumSaveBytes();
	size += sizeof(GLenum);
	size += filePath.size() + 1;
	size += sizeof(int);
	size += sizeof(bool);
	if (!source.empty()) {
		size += source.length() + 1;
	}
	else {
		size += 1; // For the null terminator
	}
	return size;
}

int ShaderProgram::getNumSaveBytes() {
	int size = 0;
	size += Resource::getNumSaveBytes();
	size += sizeof(bool);
	for(int i = 0; i < eVAttribute_MAX; ++i) {
		size += shaderAttributeNames[i].size() + 1;
	}
	for(int i = 0; i < eSUniform_MAX; ++i) {
		size += shaderUniformNames[i].size() + 1;
	}
	for(int i = 0; i < eMaterialTexture_MAX; ++i) {
		size += shaderTextureNames[i].size() + 1;
		size += shaderOutputNames[i].size() + 1;
	}

	return size;
}

void ShaderProgram::finalizeLoad(inst_conv_map* conversionMap) {
	Resource::finalizeLoad(conversionMap);
	createProgram();
}

Instance* ShaderProgram::clone_this() {
	ShaderProgram* s = (ShaderProgram*)registerInstance(new ShaderProgram(*this), true);
	s->id = 0;
	s->loadInterleaved = loadInterleaved;
	for (int i = 0; i < eVAttribute_MAX; ++i) {
		s->shaderAttributeNames[i] = shaderAttributeNames[i];
	}
	for (int i = 0; i < eSUniform_MAX; ++i) {
		s->shaderUniformNames[i] = shaderUniformNames[i];
	}
	for (int i = 0; i < eMaterialTexture_MAX; ++i) {
		s->shaderTextureNames[i] = shaderTextureNames[i];
		s->shaderOutputNames[i] = shaderOutputNames[i];
	}
	return s;
}