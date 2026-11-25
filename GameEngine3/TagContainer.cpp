#include "TagContainer.h"
#include "Graphics/GraphicsWrapper.h"


#include "GameInstance.h"
TagContainer::TagContainer() {
	tags.clear();
}

bool TagContainer::hasTag(std::string tag) {
	return tags.find(tag) != tags.end();
}

void TagContainer::addTag(std::string tag) {
	tags.insert(tag);
}

void TagContainer::removeTag(std::string tag) {
	tags.erase(tag);
}

std::vector<std::string> TagContainer::getTagsAsVector() {
	return std::vector<std::string>(tags.begin(), tags.end());
}

int TagContainer::writeToFile(FILE* file) {
	int offset = 0;
	int numTags = (int)tags.size();
	offset += fwrite(&numTags, sizeof(int), 1, file);
	for (const std::string& tag : tags) {
		offset += fwrite(tag.c_str(), sizeof(char), tag.size() + 1, file);
	}
	return offset;
}

int TagContainer::loadFromFile(void* data, int off) {
	int o = off;
	int numTags = *(int*)offset(data, o);
	o += sizeof(int);
	tags.clear();
	for (int i = 0; i < numTags; ++i) {
		std::string tag((char*)offset(data, o));
		tags.insert(tag);
		o += tag.size() + 1;
	}
	return o - off;
}

int TagContainer::getNumSaveBytes() {
	int size = sizeof(int);
	for (const std::string& tag : tags) {
		size += tag.size() + 1;
	}
	return size;
}