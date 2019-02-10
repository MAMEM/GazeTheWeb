//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Helper functions for general usage.

#ifndef HELPER_H_
#define HELPER_H_

#include "src/Utils/glmWrapper.h"
#include <string>
#include <vector>

// Convert color [0..1] to hexadecimal string
std::string RGBAToHexString(glm::vec4 color);

// Shorten URL to minimal  (https://www.google.com -> google.com)
std::string ShortenURL(std::string URL);

// Calculate maximal level of mip map
int MaximalMipMapLevel(int width, int height);

// Helper to extract float from string
float StringToFloat(std::string value);

// Split string by separator
std::vector<std::string> SplitBySeparator(std::string str, char separator);

// Simple getter of date
std::string GetDate();

// Simple getter of timestamp (in miliseconds)
std::string GetTimestamp();

// Get distance of two different strings, if usePhonetic is true soundex algorithm is used otherwise levenshtein algorithm
size_t StringDistance(const std::string s1, const std::string s2, bool usePhonetic);

#endif // HELPER_H_
