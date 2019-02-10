//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "Helper.h"
#include "src/Setup.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <ctime>
#include <chrono>
#include "submodules/eyeGUI/externals/levenshtein-sse/levenshtein-sse.hpp"


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
	for (int j = numbersCount - 1; j >= 0; j--)
	{
		result += glm::pow(10.f, dotIndex - j - 1) * numbers.at(j);
	}

	// Make it negative if necessary
	if (negative) { result = -result; }

	// Return result
	return result;
}

std::vector<std::string> SplitBySeparator(std::string str, char separator)
{
	std::vector<std::string> output;
	std::vector<char> buffer;

	// Go over string
	for (int i = 0; i < (int)str.length(); i++)
	{
		// Read single character
		const char read = str.at(i);
		if (read == separator) // character is separator
		{
			if (buffer.size() > 0) // something is in buffer
			{
				output.push_back(std::string(buffer.begin(), buffer.end())); // add to output vector
				buffer.clear(); // clear buffer
			}
			else if (i > 0 && buffer.size() == 0) // insert empty string between two separators, but not directly after first one
			{
				output.push_back(""); // add empty output
			}
		}
		else // no separator
		{
			buffer.push_back(read); // just collect the character
		}
	}

	// Add remaining characters to output
	if (buffer.size() > 0)
	{
		output.push_back(std::string(buffer.begin(), buffer.end())); // add to output vector
		buffer.clear(); // clear buffer
	}

	// Return output
	return output;
}

std::string GetDate()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::ostringstream oss;
	oss << std::put_time(&tm, setup::DATE_FORMAT.c_str());
	return oss.str();
}

std::string GetTimestamp()
{
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	return std::to_string(ms.count());
}

std::string GenerateSoundexCode(std::string input) {
	

	// implementation of the soundex algortihm (american soundex) [https://en.wikipedia.org/wiki/Soundex]
	// TODO: maybe implement alternative behaviour for different languages (needs different rating of appearing chars) 


	// initialization
	std::string soundexCode = "";
	soundexCode[0] = input[0];

	int inputCount = 1;
	int codeCount = 1;
	bool lastWasVowel = false;

	// generate the soundex code by rating the current char in the input string
	while ((inputCount < input.length()) && (codeCount < 4))
	{
		if (((input[inputCount] == 'b') 
			|| (input[inputCount] == 'p') 
			|| (input[inputCount] == 'v') 
			|| (input[inputCount] == 'f')) 
			&& (soundexCode[codeCount - 1] != 1) || lastWasVowel)	// in soundex code two adjacent with the same rating will only be counted once except a vowel is in between them
		{
			soundexCode[codeCount] = 1;
			codeCount++;
			lastWasVowel = false;
		}
		else if (((input[inputCount] == 'c') 
				|| (input[inputCount] == 'g') 
				|| (input[inputCount] == 'j') 
				|| (input[inputCount] == 'k') 
				|| (input[inputCount] == 'q') 
				|| (input[inputCount] == 's') 
				|| (input[inputCount] == 'x') 
				|| (input[inputCount] == 'z')) 
				&& (soundexCode[codeCount - 1] != 2) || lastWasVowel)
		{
			soundexCode[codeCount] = 2;
			codeCount++;
			lastWasVowel = false;
		}
		else if (((input[inputCount] == 'd') 
				|| (input[inputCount] == 't')) 
				&& (soundexCode[codeCount - 1] != 3) || lastWasVowel)
		{
			soundexCode[codeCount] = 3;
			codeCount++;
			lastWasVowel = false;
		}
		else if ((input[inputCount] == 'l') 
				&& (soundexCode[codeCount - 1] != 4) || lastWasVowel)
		{
			soundexCode[codeCount] = 4;
			codeCount++;
			lastWasVowel = false;
		}
		else if (((input[inputCount] == 'm') 
				|| (input[inputCount] == 'n')) 
				&& (soundexCode[codeCount - 1] != 5) || lastWasVowel)
		{
			soundexCode[codeCount] = 5;
			codeCount++;
			lastWasVowel = false;
		}
		else if ((input[inputCount] == 'r') 
				&& (soundexCode[codeCount - 1] != 6) || lastWasVowel)
		{
			soundexCode[codeCount] = 6;
			codeCount++;
			lastWasVowel = false;
		}
		else if ((input[inputCount] == 'a')
				|| (input[inputCount] == 'e')
				|| (input[inputCount] == 'i')
				|| (input[inputCount] == 'o')
				|| (input[inputCount] == 'u'))
		{
			lastWasVowel = true;
		}

		inputCount++;
	}

	while (codeCount < 4)
	{
		soundexCode[codeCount] = 0;
		codeCount++;
	}

	return soundexCode;
	
}

size_t StringDistance(const std::string s1, const std::string s2, bool usePhonetic)
{

	// determine distance using levenshtein if wanted
	if(!usePhonetic)
		return levenshteinSSE::levenshtein(s1, s2);

	
	size_t output = 0;

	// generate soundex codes 
	std::string firstSoundexCode = GenerateSoundexCode(s1);
	std::string secondSoundexCode = GenerateSoundexCode(s2);
	
	// check similarity between soundex codes
	for (int i = 0; i < 4; i++) {
		if (firstSoundexCode[i] != secondSoundexCode[i])
			output += 1;
	}

	return output;
}

