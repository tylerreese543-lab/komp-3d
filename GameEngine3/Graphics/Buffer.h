#pragma once
#include "graphics_include.h"
#include <stdlib.h>
class Buffer {
	public:
	GLuint id;
	GLenum target;
	GLenum usage;
	GLint element_size;
	void* data;
	GLuint size;
	Buffer();
	Buffer(GLenum target, GLsizeiptr size, GLenum usage);
	virtual ~Buffer();
	virtual int generate();
	virtual int bind();
	virtual int unbind();
	virtual int setDataSub(GLintptr offset, GLsizeiptr size, const void* data);
	virtual int setData(GLsizeiptr size, GLenum usage);
	virtual int update();
	//Calculates 'size' based off of 'element_size' and the count as an argument
	virtual void calcSize(int count);
	int getCount();
};

class GraphicsBuffer {
public:
	bool isFrameBuffer;

	GLenum type;
	GLuint id;

	GraphicsBuffer();
	virtual void generate();
	virtual void bind();
	virtual bool exists();
	virtual void unbind();
	virtual bool isFramebufferComplete();
	virtual void rebindAs(GLenum type, bool unbind = true);
};