//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "Logger.h"
#include "src/Global.h"
#include "src/Setup.h"
#include "submodules/spdlog/include/spdlog/spdlog.h"
#include <memory>

// Shared pointer of global logger
std::shared_ptr<spdlog::logger> GlobalLog;

// Definition of logger path variable
std::string LogPath;

std::shared_ptr<spdlog::logger> Log()
{
    if (!GlobalLog) // check for null
    {
        // Create logger since there is no, yet
        std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(LogPath + LOG_FILE_NAME + ".txt", LOG_FILE_MAX_SIZE, LOG_FILE_COUNT));
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
        GlobalLog = std::make_shared<spdlog::logger>("global_log", begin(sinks), end(sinks));
        GlobalLog->set_pattern("[%D-%T] %l: %v");

		// Set logging level
		if (setup::DEBUG_MODE)
		{
			GlobalLog->set_level(spdlog::level::debug);
		}
		else
		{
			GlobalLog->set_level(spdlog::level::info);
		}
    }
    return GlobalLog;
}

void LogInfo(const std::string& content)
{
	Log()->info(content);
}

void LogError(const std::string& content)
{
	Log()->error(content);
}

void LogDebug(const std::string& content)
{
	Log()->debug(content);
}

void LogBug(const std::string& content)
{
	Log()->error(content);
}

