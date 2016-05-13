#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include "Core/Containers/DynamicArray.h"

class ManifestLoader
{
public:
	ManifestLoader()
	{

	}

	~ManifestLoader()
	{

	}

	void LoadManifest(char *filename)
	{
		mFileNames.Clear();
		std::ifstream file(filename);
		std::string directory;
		std::string line;

		if(file.is_open())
		{
			std::getline(file, directory);
			while(std::getline(file, line))
			{
				std::string newFileName;
				newFileName.append(directory);
				newFileName.append(line);
				mFileNames.Add(newFileName);
			}
			file.close();
		}
	}

	DynamicArray<std::string> &GetFileNames()
	{
		return mFileNames;
	}

private:
	DynamicArray<std::string> mFileNames;
};