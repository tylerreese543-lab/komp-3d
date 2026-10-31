#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include "Resource.h"
class FileStream
{
	FILE* file;
	bool opened;
	bool reachedEndOfFile;
	bool reachedReadError;
	//false = write / true = read

	bool fileMode;
	unsigned int o;

	const static std::string saveFileLocation;
	const static std::string saveFileExtension;
	bool ok(bool mode);
	void errorCheck();
	std::string concatenatedFilePath();
public:

	bool isNew;
	std::string filePath;
	FileStream(bool mode = false);
	~FileStream();

	int open(std::string filepath = "main");
	void close();

	void writeInt(int value);
	void writeBool(bool value);
	void writeString(std::string value);
	void writeFloat(float value);

	int readInt();
	bool readBool();
	std::string readString();
	float readFloat();
};

