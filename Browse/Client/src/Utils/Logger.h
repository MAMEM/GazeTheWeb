//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Abstraction of logger. Can be called from threads, too.

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <sstream>

// Declaration of output path for log. Should be only set by main.cpp
extern std::string LogPath;

// LogInfo
void LogInfo(const std::string& content);

template<typename... Args>
void LogInfo(Args const&... args)
{
    std::ostringstream stream;
    using List= int[];
    (void)List{0, ((void)(stream << args), 0) ... };
    LogInfo(stream.str());
}

// LogError
void LogError(const std::string& content);

template<typename... Args>
void LogError(Args const&... args)
{
    std::ostringstream stream;
    using List= int[];
    (void)List{0, ((void)(stream << args), 0) ... };
    LogError(stream.str());
}

// LogDebug
void LogDebug(const std::string& content);

template<typename... Args>
void LogDebug(Args const&... args)
{
    std::ostringstream stream;
    using List= int[];
    (void)List{0, ((void)(stream << args), 0) ... };
    LogDebug(stream.str());
}

// LogBug
void LogBug(const std::string& content);

template<typename... Args>
void LogBug(Args const&... args)
{
    std::ostringstream stream;
    using List= int[];
    (void)List{0, ((void)(stream << args), 0) ... };
    LogBug(stream.str());
}

#endif // LOGGER_H_
