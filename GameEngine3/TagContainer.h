#pragma once

#include <set>
#include <string>
#include <vector>
#include <stdio.h>

class TagContainer
{
	std::set<std::string> tags;
public:
	bool hasTag(std::string tag);
	void addTag(std::string tag);
	void removeTag(std::string tag);

	std::vector<std::string> getTagsAsVector();
	TagContainer();

	int writeToFile(FILE* file);
	int loadFromFile(void* data, int offset = 0);
	int getNumSaveBytes();
};

