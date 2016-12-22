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
		std::string line;

		if(file.is_open())
		{
			std::getline(file, mDirectoryMapping);
			while(std::getline(file, line))
			{
				std::string newFileName;
				newFileName.append(mDirectoryMapping);
				newFileName.append(line);
				mFileNames.Add(newFileName);
			}
			file.close();
		}
	}

	DynamicArray<std::string, false> &GetFileNames()
	{
		return mFileNames;
	}

private:
	DynamicArray<std::string, false> mFileNames;
	std::string mDirectoryMapping;
};