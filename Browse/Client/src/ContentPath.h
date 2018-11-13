//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifndef CONTENT_PATH_H_
#define CONTENT_PATH_H_

#include <experimental/filesystem>

#ifdef CLIENT_DEPLOYMENT
static const std::string RUNTIME_CONTENT_PATH = std::experimental::filesystem::current_path().generic_string() + "/content";
#else
static const std::string RUNTIME_CONTENT_PATH = CONTENT_PATH;
#endif // CLIENT_DEPLOYMENT

#endif // CONTENT_PATH_H_