#pragma once
#include <string>
#include <fstream>

class FileUtil
{
public:
	static bool DoesFileExist(const std::string &filePath)
	{
		return DoesFileExist(filePath.c_str());
	}

	static bool DoesFileExist(const char *filePath)
	{
		std::ifstream f(filePath);

		if (f.good())
		{
			f.close();
			return true;
		}
		else
		{
			f.close();
			return false;
		}

		return false;
	}
};