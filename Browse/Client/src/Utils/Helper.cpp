//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "Helper.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>

std::string RGBAToHexString(glm::vec4 color)
{
    // Clamp color
    glm::clamp(color, glm::vec4(0,0,0,0), glm::vec4(1,1,1,1));

    // Get 8-Bits out of it
    unsigned int r = (unsigned int) (color.r * 255);
    unsigned int g = (unsigned int) (color.g * 255);
    unsigned int b = (unsigned int) (color.b * 255);
    unsigned int a = (unsigned int) (color.a * 255);

    // Create hexadecimal out of it
    unsigned int hexNumber = ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff);

    // Make string out of it
    std::stringstream ss;
    ss << std::showbase // show the 0x prefix
        << std::internal // fill between the prefix and the number
        << std::setfill('0'); // set filling
    ss << std::hex << std::setw(10) << hexNumber; // create 10 chars long string
    return ss.str();
}

std::string ShortenURL(std::string URL)
{
	// Get rid of http
	std::string delimiter = "http://";
	size_t pos = 0;
	if((pos = URL.find(delimiter)) != std::string::npos)
	{
		URL.erase(0, pos + delimiter.length());
	}

	// Get rid of https
	delimiter = "https://";
	pos = 0;
	if((pos = URL.find(delimiter)) != std::string::npos)
	{
		URL.erase(0, pos + delimiter.length());
	}

	// Get rid of www.
	delimiter = "www.";
	pos = 0;
	if((pos = URL.find(delimiter)) != std::string::npos)
	{
		URL.erase(0, pos + delimiter.length());
	}

	// Get rid of everything beyond and inclusive /
	delimiter = "/";
	pos = 0;
	std::string shortURL = URL;
	if((pos = URL.find(delimiter)) != std::string::npos)
	{
		return URL.substr(0, pos);
	}
	else
	{
		return URL;
	}
}

int MaximalMipMapLevel(int width, int height)
{
	return std::floor(std::log2(std::fmax((float)width, (float)height)));
}

float StringToFloat(std::string value)
{
	// Find out whether negative
	int i = 0;
	bool negative = false;
	if (!value.empty())
	{
		if (value.at(0) == '-')
		{
			i = 1; // first char is minus, so start at second
			negative = true;
		}
	}

	// Ugly but more portable than C++11 converter functions which may use locale of computer
	std::vector<int> numbers;
	int dotIndex = -1;
	for (; i < (int)value.length(); i++)
	{
		// Fetch character
		char c = value.at(i);

		// Decide what to do with it
		switch (c)
		{
		case '0':
			numbers.push_back(0);
			break;
		case '1':
			numbers.push_back(1);
			break;
		case '2':
			numbers.push_back(2);
			break;
		case '3':
			numbers.push_back(3);
			break;
		case '4':
			numbers.push_back(4);
			break;
		case '5':
			numbers.push_back(5);
			break;
		case '6':
			numbers.push_back(6);
			break;
		case '7':
			numbers.push_back(7);
			break;
		case '8':
			numbers.push_back(8);
			break;
		case '9':
			numbers.push_back(9);
			break;
		case '.':
			if (dotIndex >= 0)
			{
				// More than one dot in a floating point number is no good
				return -1.f;
			}
			else
			{
				// Remember index where dot appeared
				dotIndex = negative ? i - 1 : i; // if negative, subtract index of negative sign
			}
			break;
		default:
			return -1.f;
		}
	}

	// Build floating point
	int numbersCount = numbers.size();
	if (dotIndex < 0) { dotIndex = numbersCount; }
	float result = 0;
	for (int i = numbersCount - 1; i >= 0; i--)
	{
		result += glm::pow(10.f, dotIndex - i - 1) * numbers.at(i);
	}

	// Make it negative if necessary
	if (negative) { result = -result; }

	// Return result
	return result;
}
