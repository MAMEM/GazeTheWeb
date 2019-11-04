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
#include <vector>
#include <string>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

std::string RGBAToHexString(glm::vec4 color)
{
	// Clamp color
	glm::clamp(color, glm::vec4(0, 0, 0, 0), glm::vec4(1, 1, 1, 1));

	// Get 8-Bits out of it
	unsigned int r = (unsigned int)(color.r * 255);
	unsigned int g = (unsigned int)(color.g * 255);
	unsigned int b = (unsigned int)(color.b * 255);
	unsigned int a = (unsigned int)(color.a * 255);

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
	if ((pos = URL.find(delimiter)) != std::string::npos)
	{
		URL.erase(0, pos + delimiter.length());
	}

	// Get rid of https
	delimiter = "https://";
	pos = 0;
	if ((pos = URL.find(delimiter)) != std::string::npos)
	{
		URL.erase(0, pos + delimiter.length());
	}

	// Get rid of www.
	delimiter = "www.";
	pos = 0;
	if ((pos = URL.find(delimiter)) != std::string::npos)
	{
		URL.erase(0, pos + delimiter.length());
	}

	// Get rid of everything beyond and inclusive /
	delimiter = "/";
	pos = 0;
	std::string shortURL = URL;
	if ((pos = URL.find(delimiter)) != std::string::npos)
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


// Metaphone algorithm (maybe in seperate file for readability?)

const unsigned int max_length = 32;

void MakeUpper(std::string &s) {
	for (unsigned int i = 0; i < s.length(); i++) {
		s[i] = toupper(s[i]);
	}
}

int IsVowel(std::string &s, unsigned int pos)
{
	char c;

	if ((pos < 0) || (pos >= s.length()))
		return 0;

	c = s[pos];
	if ((c == 'A') || (c == 'E') || (c == 'I') || (c == 'O') ||
		(c == 'U') || (c == 'Y')) {
		return 1;
	}

	return 0;
}

int SlavoGermanic(std::string &s)
{
	if ((char *)strstr(s.c_str(), "W"))
		return 1;
	else if ((char *)strstr(s.c_str(), "K"))
		return 1;
	else if ((char *)strstr(s.c_str(), "CZ"))
		return 1;
	else if ((char *)strstr(s.c_str(), "WITZ"))
		return 1;
	else
		return 0;
}

char GetAt(std::string &s, unsigned int pos)
{
	if ((pos < 0) || (pos >= s.length())) {
		return '\0';
	}

	return s[pos];
}

void SetAt(std::string &s, unsigned int pos, char c)
{
	if ((pos < 0) || (pos >= s.length())) {
		return;
	}

	s[pos] = c;
}

/*
  Caveats: the START value is 0 based
*/
int StringAt(std::string &s, unsigned int start, unsigned int length, ...)
{
	char *test;
	const char *pos;
	va_list ap;

	if ((start < 0) || (start >= s.length())) {
		return 0;
	}

	pos = (s.c_str() + start);
	va_start(ap, length);

	do {
		test = va_arg(ap, char *);
		if (*test && (strncmp(pos, test, length) == 0)) {
			return 1;
		}
	} while (strcmp(test, ""));

	va_end(ap);

	return 0;
}

void CreateBitmapFile(unsigned char const * pBuffer, int width, int height, int bytesPerPixel, char* fName) {

	unsigned char padding[3] = { 0, 0, 0 };
	int paddingSize = (4 - (width*bytesPerPixel) % 4) % 4;
	int filesize = 54 + 4 * width*height;
	unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
	unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 32,0 };

	bmpfileheader[2] = (unsigned char)(filesize);
	bmpfileheader[3] = (unsigned char)(filesize >> 8);
	bmpfileheader[4] = (unsigned char)(filesize >> 16);
	bmpfileheader[5] = (unsigned char)(filesize >> 24);

	bmpinfoheader[4] = (unsigned char)(width);
	bmpinfoheader[5] = (unsigned char)(width >> 8);
	bmpinfoheader[6] = (unsigned char)(width >> 16);
	bmpinfoheader[7] = (unsigned char)(width >> 24);
	bmpinfoheader[8] = (unsigned char)(height);
	bmpinfoheader[9] = (unsigned char)(height >> 8);
	bmpinfoheader[10] = (unsigned char)(height >> 16);
	bmpinfoheader[11] = (unsigned char)(height >> 24);

	FILE* imageFile = fopen(fName, "wb");

	fwrite(bmpfileheader, 1, 14, imageFile);
	fwrite(bmpinfoheader, 1, 40, imageFile);

	int i;
	for (i = height - 1; i >= 0; i--) {
		fwrite(pBuffer + (i*width*bytesPerPixel), bytesPerPixel, width, imageFile);
		fwrite(padding, 1, paddingSize, imageFile);
	}
	fclose(imageFile);
}

void DoubleMetaphone(const std::string &str, std::vector<std::string> *codes)
{
	int        length;
	std::string original;
	std::string primary;
	std::string secondary;
	int        current;
	int        last;

	current = 0;
	/* we need the real length and last prior to padding */
	length = str.length();
	last = length - 1;
	original = str; // make a copy
	/* Pad original so we can index beyond end */
	original += "     ";

	primary = "";
	secondary = "";

	MakeUpper(original);

	/* skip these when at start of word */
	if (StringAt(original, 0, 2, "GN", "KN", "PN", "WR", "PS", "")) {
		current += 1;
	}

	/* Initial 'X' is pronounced 'Z' e.g. 'Xavier' */
	if (GetAt(original, 0) == 'X') {
		primary += "S";	/* 'Z' maps to 'S' */
		secondary += "S";
		current += 1;
	}

	/* main loop */
	while ((primary.length() < max_length) || (secondary.length() < max_length)) {
		if (current >= length) {
			break;
		}

		switch (GetAt(original, current)) {
		case 'A':
		case 'E':
		case 'I':
		case 'O':
		case 'U':
		case 'Y':
			if (current == 0) {
				/* all init vowels now map to 'A' */
				primary += "A";
				secondary += "A";
			}
			current += 1;
			break;

		case 'B':
			/* "-mb", e.g", "dumb", already skipped over... */
			primary += "P";
			secondary += "P";

			if (GetAt(original, current + 1) == 'B')
				current += 2;
			else
				current += 1;
			break;

		case 'Ç':
			primary += "S";
			secondary += "S";
			current += 1;
			break;

		case 'C':
			/* various germanic */
			if ((current > 1) &&
				!IsVowel(original, current - 2) &&
				StringAt(original, (current - 1), 3, "ACH", "") &&
				((GetAt(original, current + 2) != 'I') &&
				((GetAt(original, current + 2) != 'E') ||
					StringAt(original, (current - 2), 6, "BACHER", "MACHER", "")))) {
				primary += "K";
				secondary += "K";
				current += 2;
				break;
			}

			/* special case 'caesar' */
			if ((current == 0) && StringAt(original, current, 6, "CAESAR", "")) {
				primary += "S";
				secondary += "S";
				current += 2;
				break;
			}

			/* italian 'chianti' */
			if (StringAt(original, current, 4, "CHIA", "")) {
				primary += "K";
				secondary += "K";
				current += 2;
				break;
			}

			if (StringAt(original, current, 2, "CH", "")) {
				/* find 'michael' */
				if ((current > 0) && StringAt(original, current, 4, "CHAE", "")) {
					primary += "K";
					secondary += "X";
					current += 2;
					break;
				}

				/* greek roots e.g. 'chemistry', 'chorus' */
				if ((current == 0) &&
					(StringAt(original, (current + 1), 5,
						"HARAC", "HARIS", "") ||
						StringAt(original, (current + 1), 3,
							"HOR", "HYM", "HIA", "HEM", "")) &&
					!StringAt(original, 0, 5, "CHORE", "")) {
					primary += "K";
					secondary += "K";
					current += 2;
					break;
				}

				/* germanic, greek, or otherwise 'ch' for 'kh' sound */
				if ((StringAt(original, 0, 4, "VAN ", "VON ", "") ||
					StringAt(original, 0, 3, "SCH", "")) ||
					/*  'architect but not 'arch', 'orchestra', 'orchid' */
					StringAt(original, (current - 2), 6,
						"ORCHES", "ARCHIT", "ORCHID", "") ||
					StringAt(original, (current + 2), 1,
						"T", "S", "") ||
						((StringAt(original, (current - 1), 1,
							"A", "O", "U", "E", "") ||
							(current == 0)) &&
							/* e.g., 'wachtler', 'wechsler', but not 'tichner' */
							StringAt(original, (current + 2), 1, "L", "R",
								"N", "M", "B", "H", "F", "V", "W", " ", ""))) {
					primary += "K";
					secondary += "K";
				}
				else {
					if (current > 0) {
						if (StringAt(original, 0, 2, "MC", "")) {
							/* e.g., "McHugh" */
							primary += "K";
							secondary += "K";
						}
						else {
							primary += "X";
							secondary += "K";
						}
					}
					else {
						primary += "X";
						secondary += "X";
					}
				}
				current += 2;
				break;
			}
			/* e.g, 'czerny' */
			if (StringAt(original, current, 2, "CZ", "") &&
				!StringAt(original, (current - 2), 4, "WICZ", "")) {
				primary += "S";
				secondary += "X";
				current += 2;
				break;
			}

			/* e.g., 'focaccia' */
			if (StringAt(original, (current + 1), 3, "CIA", "")) {
				primary += "X";
				secondary += "X";
				current += 3;
				break;
			}

			/* double 'C', but not if e.g. 'McClellan' */
			if (StringAt(original, current, 2, "CC", "") &&
				!((current == 1) && (GetAt(original, 0) == 'M'))) {
				/* 'bellocchio' but not 'bacchus' */
				if (StringAt(original, (current + 2), 1, "I", "E", "H", "") &&
					!StringAt(original, (current + 2), 2, "HU", "")) {
					/* 'accident', 'accede' 'succeed' */
					if (((current == 1) && (GetAt(original, current - 1) == 'A')) ||
						StringAt(original, (current - 1), 5, "UCCEE", "UCCES", "")) {
						primary += "KS";
						secondary += "KS";
						/* 'bacci', 'bertucci', other italian */
					}
					else {
						primary += "X";
						secondary += "X";
					}
					current += 3;
					break;
				}
				else {  /* Pierce's rule */
					primary += "K";
					secondary += "K";
					current += 2;
					break;
				}
			}

			if (StringAt(original, current, 2, "CK", "CG", "CQ", "")) {
				primary += "K";
				secondary += "K";
				current += 2;
				break;
			}

			if (StringAt(original, current, 2, "CI", "CE", "CY", "")) {
				/* italian vs. english */
				if (StringAt(original, current, 3, "CIO", "CIE", "CIA", "")) {
					primary += "S";
					secondary += "X";
				}
				else {
					primary += "S";
					secondary += "S";
				}
				current += 2;
				break;
			}

			/* else */
			primary += "K";
			secondary += "K";

			/* name sent in 'mac caffrey', 'mac gregor */
			if (StringAt(original, (current + 1), 2, " C", " Q", " G", ""))
				current += 3;
			else
				if (StringAt(original, (current + 1), 1, "C", "K", "Q", "") &&
					!StringAt(original, (current + 1), 2, "CE", "CI", ""))
					current += 2;
				else
					current += 1;
			break;

		case 'D':
			if (StringAt(original, current, 2, "DG", "")) {
				if (StringAt(original, (current + 2), 1, "I", "E", "Y", "")) {
					/* e.g. 'edge' */
					primary += "J";
					secondary += "J";
					current += 3;
					break;
				}
				else {
					/* e.g. 'edgar' */
					primary += "TK";
					secondary += "TK";
					current += 2;
					break;
				}
			}

			if (StringAt(original, current, 2, "DT", "DD", "")) {
				primary += "T";
				secondary += "T";
				current += 2;
				break;
			}

			/* else */
			primary += "T";
			secondary += "T";
			current += 1;
			break;

		case 'F':
			if (GetAt(original, current + 1) == 'F')
				current += 2;
			else
				current += 1;
			primary += "F";
			secondary += "F";
			break;

		case 'G':
			if (GetAt(original, current + 1) == 'H') {
				if ((current > 0) && !IsVowel(original, current - 1)) {
					primary += "K";
					secondary += "K";
					current += 2;
					break;
				}

				if (current < 3) {
					/* 'ghislane', ghiradelli */
					if (current == 0) {
						if (GetAt(original, current + 2) == 'I') {
							primary += "J";
							secondary += "J";
						}
						else {
							primary += "K";
							secondary += "K";
						}
						current += 2;
						break;
					}
				}
				/* Parker's rule (with some further refinements) - e.g., 'hugh' */
				if (((current > 1) &&
					StringAt(original, (current - 2), 1, "B", "H", "D", "")) ||
					/* e.g., 'bough' */
					((current > 2) &&
						StringAt(original, (current - 3), 1, "B", "H", "D", "")) ||
					/* e.g., 'broughton' */
						((current > 3) &&
							StringAt(original, (current - 4), 1, "B", "H", ""))) {
					current += 2;
					break;
				}
				else {
					/* e.g., 'laugh', 'McLaughlin', 'cough', 'gough', 'rough', 'tough' */
					if ((current > 2) &&
						(GetAt(original, current - 1) == 'U') &&
						StringAt(original, (current - 3), 1, "C",
							"G", "L", "R", "T", "")) {
						primary += "F";
						secondary += "F";
					}
					else if ((current > 0) &&
						GetAt(original, current - 1) != 'I') {
						primary += "K";
						secondary += "K";
					}

					current += 2;
					break;
				}
			}

			if (GetAt(original, current + 1) == 'N') {
				if ((current == 1) &&
					IsVowel(original, 0) &&
					!SlavoGermanic(original)) {
					primary += "KN";
					secondary += "N";
				}
				else
					/* not e.g. 'cagney' */
					if (!StringAt(original, (current + 2), 2, "EY", "") &&
						(GetAt(original, current + 1) != 'Y') &&
						!SlavoGermanic(original)) {
						primary += "N";
						secondary += "KN";
					}
					else {
						primary += "KN";
						secondary += "KN";
					}
				current += 2;
				break;
			}

			/* 'tagliaro' */
			if (StringAt(original, (current + 1), 2, "LI", "") &&
				!SlavoGermanic(original)) {
				primary += "KL";
				secondary += "L";
				current += 2;
				break;
			}

			/* -ges-,-gep-,-gel-, -gie- at beginning */
			if ((current == 0) &&
				((GetAt(original, current + 1) == 'Y') ||
					StringAt(original, (current + 1), 2, "ES", "EP",
						"EB", "EL", "EY", "IB", "IL", "IN", "IE",
						"EI", "ER", ""))) {
				primary += "K";
				secondary += "J";
				current += 2;
				break;
			}

			/*  -ger-,  -gy- */
			if ((StringAt(original, (current + 1), 2, "ER", "") ||
				(GetAt(original, current + 1) == 'Y')) &&
				!StringAt(original, 0, 6, "DANGER", "RANGER", "MANGER", "") &&
				!StringAt(original, (current - 1), 1, "E", "I", "") &&
				!StringAt(original, (current - 1), 3, "RGY", "OGY", "")) {
				primary += "K";
				secondary += "J";
				current += 2;
				break;
			}

			/*  italian e.g, 'biaggi' */
			if (StringAt(original, (current + 1), 1, "E", "I", "Y", "") ||
				StringAt(original, (current - 1), 4, "AGGI", "OGGI", "")) {
				/* obvious germanic */
				if ((StringAt(original, 0, 4, "VAN ", "VON ", "") ||
					StringAt(original, 0, 3, "SCH", "")) ||
					StringAt(original, (current + 1), 2, "ET", ""))
				{
					primary += "K";
					secondary += "K";
				}
				else {
					/* always soft if french ending */
					if (StringAt(original, (current + 1), 4, "IER ", "")) {
						primary += "J";
						secondary += "J";
					}
					else {
						primary += "J";
						secondary += "K";
					}
				}
				current += 2;
				break;
			}

			if (GetAt(original, current + 1) == 'G')
				current += 2;
			else
				current += 1;
			primary += "K";
			secondary += "K";
			break;

		case 'H':
			/* only keep if first & before vowel or btw. 2 vowels */
			if (((current == 0) ||
				IsVowel(original, current - 1)) &&
				IsVowel(original, current + 1)) {
				primary += "H";
				secondary += "H";
				current += 2;
			}
			else		/* also takes care of 'HH' */
				current += 1;
			break;

		case 'J':
			/* obvious spanish, 'jose', 'san jacinto' */
			if (StringAt(original, current, 4, "JOSE", "") ||
				StringAt(original, 0, 4, "SAN ", "")) {
				if (((current == 0) && (GetAt(original, current + 4) == ' ')) ||
					StringAt(original, 0, 4, "SAN ", "")) {
					primary += "H";
					secondary += "H";
				}
				else {
					primary += "J";
					secondary += "H";
				}
				current += 1;
				break;
			}

			if ((current == 0) && !StringAt(original, current, 4, "JOSE", "")) {
				primary += "J";	/* Yankelovich/Jankelowicz */
				secondary += "A";
			}
			else {
				/* spanish pron. of e.g. 'bajador' */
				if (IsVowel(original, current - 1) &&
					!SlavoGermanic(original) &&
					((GetAt(original, current + 1) == 'A') ||
					(GetAt(original, current + 1) == 'O'))) {
					primary += "J";
					secondary += "H";
				}
				else {
					if (current == last) {
						primary += "J";
						secondary += "";
					}
					else {
						if (!StringAt(original, (current + 1), 1,
							"L", "T", "K", "S", "N", "M", "B", "Z", "") &&
							!StringAt(original, (current - 1), 1, "S", "K", "L", "")) {
							primary += "J";
							secondary += "J";
						}
					}
				}
			}

			if (GetAt(original, current + 1) == 'J')	/* it could happen! */
				current += 2;
			else
				current += 1;
			break;

		case 'K':
			if (GetAt(original, current + 1) == 'K')
				current += 2;
			else
				current += 1;
			primary += "K";
			secondary += "K";
			break;

		case 'L':
			if (GetAt(original, current + 1) == 'L') {
				/* spanish e.g. 'cabrillo', 'gallegos' */
				if (((current == (length - 3)) &&
					StringAt(original, (current - 1), 4,
						"ILLO", "ILLA", "ALLE", "")) ||
						((StringAt(original, (last - 1), 2, "AS", "OS", "") ||
							StringAt(original, last, 1, "A", "O", "")) &&
							StringAt(original, (current - 1), 4, "ALLE", ""))) {
					primary += "L";
					secondary += "";
					current += 2;
					break;
				}
				current += 2;
			}
			else
				current += 1;
			primary += "L";
			secondary += "L";
			break;

		case 'M':
			if ((StringAt(original, (current - 1), 3, "UMB", "") &&
				(((current + 1) == last) ||
					StringAt(original, (current + 2), 2, "ER", ""))) ||
				/* 'dumb','thumb' */
					(GetAt(original, current + 1) == 'M')) {
				current += 2;
			}
			else {
				current += 1;
			}
			primary += "M";
			secondary += "M";
			break;

		case 'N':
			if (GetAt(original, current + 1) == 'N') {
				current += 2;
			}
			else {
				current += 1;
			}
			primary += "N";
			secondary += "N";
			break;

		case 'Ñ':
			current += 1;
			primary += "N";
			secondary += "N";
			break;

		case 'P':
			if (GetAt(original, current + 1) == 'H') {
				primary += "F";
				secondary += "F";
				current += 2;
				break;
			}

			/* also account for "campbell", "raspberry" */
			if (StringAt(original, (current + 1), 1, "P", "B", ""))
				current += 2;
			else
				current += 1;
			primary += "P";
			secondary += "P";
			break;

		case 'Q':
			if (GetAt(original, current + 1) == 'Q')
				current += 2;
			else
				current += 1;
			primary += "K";
			secondary += "K";
			break;

		case 'R':
			/* french e.g. 'rogier', but exclude 'hochmeier' */
			if ((current == last) &&
				!SlavoGermanic(original) &&
				StringAt(original, (current - 2), 2, "IE", "") &&
				!StringAt(original, (current - 4), 2, "ME", "MA", "")) {
				primary += "";
				secondary += "R";
			}
			else {
				primary += "R";
				secondary += "R";
			}

			if (GetAt(original, current + 1) == 'R')
				current += 2;
			else
				current += 1;
			break;

		case 'S':
			/* special cases 'island', 'isle', 'carlisle', 'carlysle' */
			if (StringAt(original, (current - 1), 3, "ISL", "YSL", "")) {
				current += 1;
				break;
			}

			/* special case 'sugar-' */
			if ((current == 0) && StringAt(original, current, 5, "SUGAR", "")) {
				primary += "X";
				secondary += "S";
				current += 1;
				break;
			}

			if (StringAt(original, current, 2, "SH", "")) {
				/* germanic */
				if (StringAt(original, (current + 1), 4,
					"HEIM", "HOEK", "HOLM", "HOLZ", "")) {
					primary += "S";
					secondary += "S";
				}
				else {
					primary += "X";
					secondary += "X";
				}
				current += 2;
				break;
			}

			/* italian & armenian */
			if (StringAt(original, current, 3, "SIO", "SIA", "") ||
				StringAt(original, current, 4, "SIAN", "")) {
				if (!SlavoGermanic(original)) {
					primary += "S";
					secondary += "X";
				}
				else {
					primary += "S";
					secondary += "S";
				}
				current += 3;
				break;
			}

			/* german & anglicisations, e.g. 'smith' match 'schmidt', 'snider' match 'schneider'
			   also, -sz- in slavic language altho in hungarian it is pronounced 's' */
			if (((current == 0) &&
				StringAt(original, (current + 1), 1, "M", "N", "L", "W", "")) ||
				StringAt(original, (current + 1), 1, "Z", "")) {
				primary += "S";
				secondary += "X";
				if (StringAt(original, (current + 1), 1, "Z", ""))
					current += 2;
				else
					current += 1;
				break;
			}

			if (StringAt(original, current, 2, "SC", "")) {
				/* Schlesinger's rule */
				if (GetAt(original, current + 2) == 'H') {
					/* dutch origin, e.g. 'school', 'schooner' */
					if (StringAt(original, (current + 3), 2,
						"OO", "ER", "EN", "UY", "ED", "EM", "")) {
						/* 'schermerhorn', 'schenker' */
						if (StringAt(original, (current + 3), 2, "ER", "EN", "")) {
							primary += "X";
							secondary += "SK";
						}
						else {
							primary += "SK";
							secondary += "SK";
						}
						current += 3;
						break;
					}
					else {
						if ((current == 0) && !IsVowel(original, 3) &&
							(GetAt(original, 3) != 'W')) {
							primary += "X";
							secondary += "S";
						}
						else {
							primary += "X";
							secondary += "X";
						}
						current += 3;
						break;
					}
				}

				if (StringAt(original, (current + 2), 1, "I", "E", "Y", "")) {
					primary += "S";
					secondary += "S";
					current += 3;
					break;
				}
				/* else */
				primary += "SK";
				secondary += "SK";
				current += 3;
				break;
			}

			/* french e.g. 'resnais', 'artois' */
			if ((current == last) &&
				StringAt(original, (current - 2), 2, "AI", "OI", "")) {
				primary += "";
				secondary += "S";
			}
			else {
				primary += "S";
				secondary += "S";
			}

			if (StringAt(original, (current + 1), 1, "S", "Z", ""))
				current += 2;
			else
				current += 1;
			break;

		case 'T':
			if (StringAt(original, current, 4, "TION", "")) {
				primary += "X";
				secondary += "X";
				current += 3;
				break;
			}

			if (StringAt(original, current, 3, "TIA", "TCH", "")) {
				primary += "X";
				secondary += "X";
				current += 3;
				break;
			}

			if (StringAt(original, current, 2, "TH", "") ||
				StringAt(original, current, 3, "TTH", "")) {
				/* special case 'thomas', 'thames' or germanic */
				if (StringAt(original, (current + 2), 2, "OM", "AM", "") ||
					StringAt(original, 0, 4, "VAN ", "VON ", "") ||
					StringAt(original, 0, 3, "SCH", "")) {
					primary += "T";
					secondary += "T";
				}
				else {
					primary += "0"; /* yes, zero */
					secondary += "T";
				}
				current += 2;
				break;
			}

			if (StringAt(original, (current + 1), 1, "T", "D", "")) {
				current += 2;
			}
			else {
				current += 1;
			}
			primary += "T";
			secondary += "T";
			break;

		case 'V':
			if (GetAt(original, current + 1) == 'V') {
				current += 2;
			}
			else {
				current += 1;
			}
			primary += "F";
			secondary += "F";
			break;

		case 'W':
			/* can also be in middle of word */
			if (StringAt(original, current, 2, "WR", "")) {
				primary += "R";
				secondary += "R";
				current += 2;
				break;
			}

			if ((current == 0) &&
				(IsVowel(original, current + 1) ||
					StringAt(original, current, 2, "WH", ""))) {
				/* Wasserman should match Vasserman */
				if (IsVowel(original, current + 1)) {
					primary += "A";
					secondary += "F";
				}
				else {
					/* need Uomo to match Womo */
					primary += "A";
					secondary += "A";
				}
			}

			/* Arnow should match Arnoff */
			if (((current == last) && IsVowel(original, current - 1)) ||
				StringAt(original, (current - 1), 5,
					"EWSKI", "EWSKY", "OWSKI", "OWSKY", "") ||
				StringAt(original, 0, 3, "SCH", "")) {
				primary += "";
				secondary += "F";
				current += 1;
				break;
			}

			/* polish e.g. 'filipowicz' */
			if (StringAt(original, current, 4, "WICZ", "WITZ", "")) {
				primary += "TS";
				secondary += "FX";
				current += 4;
				break;
			}

			/* else skip it */
			current += 1;
			break;

		case 'X':
			/* french e.g. breaux */
			if (!((current == last) &&
				(StringAt(original, (current - 3), 3, "IAU", "EAU", "") ||
					StringAt(original, (current - 2), 2, "AU", "OU", "")))) {
				primary += "KS";
				secondary += "KS";
			}


			if (StringAt(original, (current + 1), 1, "C", "X", ""))
				current += 2;
			else
				current += 1;
			break;

		case 'Z':
			/* chinese pinyin e.g. 'zhao' */
			if (GetAt(original, current + 1) == 'H') {
				primary += "J";
				secondary += "J";
				current += 2;
				break;
			}
			else if (StringAt(original, (current + 1), 2, "ZO", "ZI", "ZA", "") ||
				(SlavoGermanic(original) &&
				((current > 0) &&
					GetAt(original, current - 1) != 'T'))) {
				primary += "S";
				secondary += "TS";
			}
			else {
				primary += "S";
				secondary += "S";
			}

			if (GetAt(original, current + 1) == 'Z')
				current += 2;
			else
				current += 1;
			break;

		default:
			current += 1;
		}
		/* printf("PRIMARY: %s\n", primary.str);
		   printf("SECONDARY: %s\n", secondary.str);  */
	}


	if (primary.length() > max_length)
		SetAt(primary, max_length, '\0');

	if (secondary.length() > max_length)
		SetAt(secondary, max_length, '\0');

	codes->push_back(primary);
	codes->push_back(secondary);
}

/*
	Function to evaluate the distance between 2 strings (s1,s2) with the required method (stringDistanceType)
	s1, s2 strings to compare
	StringDistanceType:
	LEVENSHTEIN: use the levenshtein algorithm, returns the levenshtein distance,
	SOUNDEX: use the soundex algorithm, returns the distance between the respective soundex codes,
	DOUBLE_METAPHONE: use the double metaphone algorithm
		returns:
			0 if the primary codes match,
			1 if any strings code matches any of the other strings codes,
			UINT_MAX if none of the codes match another code
*/
size_t StringDistance(const std::string s1, const std::string s2, StringDistanceType stringDistanceType)
{
	switch (stringDistanceType) {
	case StringDistanceType::LEVENSHTEIN:
	{
		return levenshteinSSE::levenshtein(s1, s2);
	}
	case StringDistanceType::SOUNDEX:
	{
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
	case StringDistanceType::DOUBLE_METAPHONE:
	{
		// Generate Metaphone Codes
		std::vector<std::string> firstCodes;
		std::vector<std::string> secondCodes;
		DoubleMetaphone(s1, &firstCodes);
		DoubleMetaphone(s2, &secondCodes);

		if (firstCodes[0] == secondCodes[0])
			return 0;
		else if (firstCodes[1] == secondCodes[0])
			return 1;
		else if (firstCodes[0] == secondCodes[1])
			return 1;
		else if (firstCodes[1] == secondCodes[0])
			return 1;
		else
			return 2;
		break;
	}
	default:
	{
		break;
	}
	}
}
