#pragma once
#include <string>
#include "Core/Misc/Color.h"
#include "Core/Vector/Vector3.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include "Core/Platform/PlatformCore.h"

class StringConverter
{
public:
	static void StringToWCHAR(std::string &str, wchar_t *wstr, uint32 sizeOfDestination)
	{
		const char *cstr = str.c_str();
		size_t wcharSize = strlen(cstr) + 1;
		Application::Assert((uint32)wcharSize <= sizeOfDestination);
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, wstr, wcharSize, cstr, _TRUNCATE);
	}

	static WCHAR *StringToWCHARAlloc(std::string &str)
	{
		const char *cstr = str.c_str();
		size_t newsize = strlen(cstr) + 1;

		wchar_t * wcstring = new wchar_t[newsize];
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, wcstring, newsize, cstr, _TRUNCATE);

		return wcstring;
	}

	static WCHAR *CStringToWCHARAlloc(const char *cstr)
	{
		size_t newsize = strlen(cstr) + 1;

		wchar_t * wcstring = new wchar_t[newsize];
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, wcstring, newsize, cstr, _TRUNCATE);

		return wcstring;
	}

	static float StringToFloat(std::string &str)
	{
		return (float)atof(str.c_str());
	}

	static float CStringToFloat(const char *cstr)
	{
		return (float)atof(cstr);
	}

	static Color StringToColor(std::string &str)
	{
		float color[4];
		int index = 0;

		char *cstr = (char*)str.c_str();
		char *piece;
		char *nextToken = NULL;

		piece = strtok_s(cstr, ",", &nextToken);
		while (piece != NULL && index < 4)
		{
			color[index] = CStringToFloat(piece);
			index++;
			piece = strtok_s(NULL, ",", &nextToken);
		}

		return Color(color[0], color[1], color[2], color[3]);
	}

	static Vector4 StringToVector4(std::string &str)
	{
		float vector4[4];
		int index = 0;

		char *cstr = (char*)str.c_str();
		char *piece;
		char *nextToken = NULL;

		piece = strtok_s(cstr, ",", &nextToken);
		while (piece != NULL && index < 4)
		{
			vector4[index] = CStringToFloat(piece);
			index++;
			piece = strtok_s(NULL, ",", &nextToken);
		}

		return Vector4(vector4[0], vector4[1], vector4[2], vector4[3]);
	}

	static Vector3 StringToVector3(std::string &str)
	{
		float vector3[3];
		int index = 0;

		char *cstr = (char*)str.c_str();
		char *piece;
		char *nextToken = NULL;

		piece = strtok_s(cstr, ",", &nextToken);
		while (piece != NULL && index < 3)
		{
			vector3[index] = CStringToFloat(piece);
			index++;
			piece = strtok_s(NULL, ",", &nextToken);
		}

		return Vector3(vector3[0], vector3[1], vector3[2]);
	}

	static int StringToInt(std::string &str)
	{
		return atoi(str.c_str());
	}

	static std::string IntToString(int val)
	{
		std::ostringstream stream;
		if (!(stream << val))
		{
			throw new std::exception("Error converting value to string");
		}
		return stream.str();
	}

	static std::string FloatToString(float val)
	{
		std::ostringstream stream;
		if (!(stream << val))
		{
			throw new std::exception("Error converting value to string");
		}
		return stream.str();
	}

	static std::string Vector3ToString(Vector3 val)
	{
		std::ostringstream stream;
		
		stream << val.X;
		stream << ",";
		stream << val.Y;
		stream << ",";
		stream << val.Z;

		return stream.str();
	}

	static std::string Vector4ToString(Vector4 val)
	{
		std::ostringstream stream;

		stream << val.X;
		stream << ",";
		stream << val.Y;
		stream << ",";
		stream << val.Z;
		stream << ",";
		stream << val.W;

		return stream.str();
	}

	static std::string ColorToString(Color val)
	{
		std::ostringstream stream;

		stream << val.R;
		stream << ",";
		stream << val.G;
		stream << ",";
		stream << val.B;
		stream << ",";
		stream << val.A;

		return stream.str();
	}

	static void RemoveCharsFromString(std::string &str, char *chars)
	{
		for (unsigned int i = 0; i < strlen(chars); ++i)
		{
			str.erase(std::remove(str.begin(), str.end(), chars[i]), str.end());
		}
	}
};