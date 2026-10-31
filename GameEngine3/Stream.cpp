#include "Stream.h"
#include "Graphics/GraphicsWrapper.h"
#include <filesystem>
const std::string FileStream::saveFileLocation = "saves://";
const std::string FileStream::saveFileExtension = ".bin";

FileStream::FileStream(bool mode) {
	filePath = "";
	opened = false;
	reachedEndOfFile = false;
	file = nullptr;
	o = 0;
	fileMode = mode;
}

FileStream::~FileStream() {
	close();
}
int FileStream::open(std::string filepath) {
	if (filepath == filePath) return -1;
	if (opened || filepath == "") close();

	filePath = filepath;
	isNew = !fileMode && std::filesystem::exists(concatenatedFilePath());
	fopen_s(&file, Resource::resolveFilePath(concatenatedFilePath()).c_str(), fileMode ? "rb" : "wb");
	if (!file) {
		opened = false;
		reachedReadError = true;
		debug_log("Unable to open stream file %s\n", filePath.c_str());
		file = nullptr;
		return -1;
	}
	else {
		o = 0;
		opened = true;
		reachedReadError = false;
		reachedEndOfFile = false;
		debug_log("Opening %s with mode %s\n", filePath.c_str(), fileMode ? "read" : "write");
	}

	return 0;
}

void FileStream::close() {
	if (opened) {
		fclose(file);
		file = nullptr;
		opened = false;
	}
}

void FileStream::errorCheck() {
	//Make sure we aren't checking a closed file;

	if (file && opened) {
		if (feof(file)) {
			debug_log("Error: operation led to EOF on file %s", filePath.c_str());
			reachedEndOfFile = true;
			close();
		}
		if (ferror(file)) {
			debug_log("Error: operation led to error on file %s", filePath.c_str());
			reachedReadError = true;
			close();
		}
	}
}
bool FileStream::ok(bool mode) {
	return fileMode == mode && !reachedEndOfFile && !reachedReadError && opened && file;
}
void FileStream::writeBool(bool value) {
	if (ok(false)) {
		o += fwrite(&value, sizeof(bool), 1, file);
	}
}
void FileStream::writeInt(int value) {
	if (ok(false)) {
		o += fwrite(&value, sizeof(int), 1, file);
	}
}
void FileStream::writeString(std::string value) {
	if (ok(false)) {
		int length = value.length() + 1;
		//Always write/read the length of the string to file first for safety
		writeInt(length);
		o += fwrite(value.c_str(), sizeof(char), length, file);
	}
}

void FileStream::writeFloat(float value) {
	if (ok(false)) {
		o += fwrite(&value, sizeof(float), 1, file);
	}
}

int FileStream::readInt() {
	errorCheck();
	int value = 0;
	if (ok(true)) {
		o += fread(&value, sizeof(int), 1, file) * sizeof(int);
	}
	return value;
}

bool FileStream::readBool() {
	errorCheck();
	bool value = 0;
	if (ok(true)) {
		o += fread(&value, sizeof(bool), 1, file) * sizeof(bool);
	}
	return value;
}
float FileStream::readFloat() {
	errorCheck();
	float value = 0;
	if (ok(true)) {
		o += fread(&value, sizeof(float), 1, file) * sizeof(float);
	}
	return value;
}
std::string FileStream::readString() {

	int length = readInt();
	errorCheck();
	std::string value;
	if (ok(true)) {
		char* buf = new char[length];
		o += fread(buf, sizeof(char), length, file) * sizeof(char);
		value = std::string(buf);
		delete[] buf;
		buf = nullptr;
	}
	return value;
}

std::string FileStream::concatenatedFilePath() {
	return saveFileLocation + filePath + saveFileExtension;
}