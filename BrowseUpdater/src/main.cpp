// Copyright 2017 Raphael Menges
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>
#include "externals/curl/include/curl/curl.h"
#include "submodules/miniz/miniz.h"
#include "submodules/miniz/miniz_zip.h"

// Constants
const std::string serverURL = "https://userpages.uni-koblenz.de/~raphaelmenges/gtw-update";
const std::string tmpZipName = "gtw_new_version.zip";
const std::string tmpUnzipDirName = "gtw_new_version";
const std::string gtwPath = std::experimental::filesystem::current_path().string() + "/Browse"; // path to GazeTheWeb-Browse folder, relative from bat file that calls the updater
const long long exitSleepMS = 2000;

// Callback for CURL to retrieve zip name
size_t CURLWriteString(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

// Callback for CURL to retrieve zip file
size_t CURLWriteFile(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t written = fwrite(ptr, size, nmemb, stream);
	std::cout << ".";
	return written;
}

// End function
int Return(std::string message)
{
	std::cout << message << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(exitSleepMS));
	return 0;
}

// Main
int main()
{
	// Welcome user
	std::cout << "################################################" << std::endl;
	std::cout << "### Updater of GazeTheWeb-Browse (version 2) ###" << std::endl;
	std::cout << "################################################" << std::endl;
	std::cout << "By Raphael Menges" << std::endl;
	std::cout << std::endl;

	// ### CHECK FOR TEMP FOLDER ###
	const std::string tmpPath = std::experimental::filesystem::temp_directory_path().string();
	if (!tmpPath.empty())
	{
		std::cout << "Temporary files will be stored in: " << tmpPath << std::endl;
	}
	else
	{
		return Return("Temporary folder not available. Exiting...");
	}
	const std::string tmpZipPath = tmpPath + tmpZipName;
	const std::string tmpUnzipPath = tmpPath + tmpUnzipDirName;

	// ### CHECK LOCAL VERSION ###

	// Read version file
	std::string versionString = "";
	std::ifstream ifs(gtwPath + "/VERSION");
	if (ifs.is_open())
	{
		versionString = std::string((std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>()));
		versionString.erase(std::remove(versionString.begin(), versionString.end(), '\n'), versionString.end()); // remove new line at end of version string
		ifs.close(); // otherwise could not be deleted later
	}
	if (versionString.empty())
	{
		return Return("Local version could no be determined. Exiting...");
	}
	std::cout << "Local version: " << versionString << std::endl;

	// ### RETRIEVE DOWNLOAD URL ###
	
	// Variables
	CURL *curl; // CURL handle
	std::string readBuffer; // buffer
	std::string downloadLink = "";

	// Initialize CURL
	curl = curl_easy_init();

	// Perform request of zip to to download
	if (curl)
	{
		// Setup CURL
		curl_easy_setopt(curl, CURLOPT_URL, std::string(serverURL + "/gtw-check.cgi?version=" + versionString).c_str()); // set address of request
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow potential redirection
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLWriteString); // use write callback
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer); // set pointer to write data into

		// Perform the request
		CURLcode res = curl_easy_perform(curl);
		
		// Check for errors
		if (res != CURLE_OK)
		{
			curl_easy_cleanup(curl);
			return Return("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)) + " Exiting...");
		}

		// Extract URL to zip to download
		std::stringstream ss(readBuffer);
		std::string token;
		int i = 0;
		while (std::getline(ss, token, '\n'))
		{
			if (i == 1 && !token.empty()) // second line and only when not empty
			{
				downloadLink = serverURL + "/" + token;
				break;
			}
			++i;
		}

		// Clean up CURL
		curl_easy_cleanup(curl);
	}
	else
	{
		return Return("CURL could not be instantiated. Exiting...");
	}

	// Only continue if download link it not empty
	if (downloadLink.empty())
	{
		return Return("No new version available for download. Exiting...");
	}
	else
	{
		std::cout << "Downloading new version: " << downloadLink;
	}

	// ### DOWNLOAD NEW VERSION INTO TMP ###
	curl = curl_easy_init();
	if (curl)
	{
		FILE *fp;
		fp = fopen(tmpZipPath.c_str(), "wb");
		if (fp != NULL) // Only continue if file is opened
		{
			// 
			curl_easy_setopt(curl, CURLOPT_URL, downloadLink.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLWriteFile);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
			CURLcode res = curl_easy_perform(curl);

			// Cleanup CURL and close file
			curl_easy_cleanup(curl);
			fclose(fp);

			// Check for errors
			if (res != CURLE_OK)
			{
				return Return("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)) + " Exiting...");
			}
		}
		else
		{
			return Return("Target path for zip download could be opened. Exiting...");
		}
		std::cout << "...download done." << std::endl;
	}
	else
	{
		return Return("CURL could not be instantiated. Exiting...");
	}

	// ### UNPACK NEW VERSION INTO TMP ##

	std::cout << "Unzipping new version";

	// Try to open archive
	mz_zip_archive zip_archive;
	memset(&zip_archive, 0, sizeof(zip_archive));
	auto status = mz_zip_reader_init_file(&zip_archive, tmpZipPath.c_str(), 0);

	// Check for success
	if (!status)
	{
		std::experimental::filesystem::remove(tmpZipPath.c_str());
		std::cout << std::endl;
		return Return("Temporary zip file could not be read. Exiting...");
	}

	// Create folder to place unzipped content
	if (std::experimental::filesystem::exists(tmpUnzipPath)) // check whether already exists
	{
		std::experimental::filesystem::remove_all(tmpUnzipPath); // remove when already existing
		
	}
	std::experimental::filesystem::create_directory(tmpUnzipPath);

	// Go over content of zip file
	for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); ++i)
	{
		// Read file stat
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
		{
			mz_zip_reader_end(&zip_archive);
			std::experimental::filesystem::remove(tmpZipPath.c_str());
			std::experimental::filesystem::remove(tmpUnzipPath.c_str());
			std::cout << std::endl;
			return Return("Temporary zip file could not be read. Exiting...");
		}
		bool isDirectory = mz_zip_reader_is_file_a_directory(&zip_archive, i);

		// Store file or create folder
		std::string path = tmpUnzipPath + "/" + file_stat.m_filename;
		if (isDirectory) // directory
		{
			if (!std::experimental::filesystem::exists(path)) // check whether already exists
			{
				std::experimental::filesystem::create_directory(path); // create the directory if not existing
			}
		}
		else // file
		{
			mz_zip_reader_extract_to_file(&zip_archive, i, path.c_str(), 0); // unzip it
		}

		// Show progress
		std::cout << ".";
	}

	// Close the archive, freeing any resources it was using
	mz_zip_reader_end(&zip_archive);

	std::cout << "...unzipping done." << std::endl;

	// Remove zip
	std::cout << "Removing temporary zip file..." << std::endl;
	std::experimental::filesystem::remove(tmpZipPath.c_str());

	// ### REPLACE OLD VERSION ###

	// Remove complete folder recursively
	std::cout << "Removing old version..." << std::endl;
	std::experimental::filesystem::remove_all(gtwPath.c_str());

	// Move new version to given path (move, not copy)
	std::cout << "Moving new version to directory..." << std::endl;
	std::experimental::filesystem::rename(tmpUnzipPath.c_str(), gtwPath.c_str());

	// ### RETURN ###

	// Return from main
	return Return("Success! Exiting updater...");
}