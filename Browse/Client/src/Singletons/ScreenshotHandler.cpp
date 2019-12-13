//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================

#pragma once

#include "ScreenshotHandler.h"
#include "src/Utils/Logger.h"
#include "src/Setup.h"

#ifdef __linux__
namespace fs = std::experimental::filesystem;
#elif _WIN32
#include <experimental/filesystem>
#include <filesystem>
namespace fs = std::experimental::filesystem::v1;
#endif

void ScreenshotHandler::SetBuffer(unsigned char const * buffer, int width, int height, int bytesPerPixel)
{
	_buffer = buffer;
	_width = width;
	_height = height;
	_bytesPerPixel = bytesPerPixel;
};

void ScreenshotHandler::SetGaze(int gazeX, int gazeY){
	_currentGazeX = gazeX;
	_currentGazeY = gazeY;
}

void ScreenshotHandler::PrepareScreenshot(bool partial, std::string path)
{
	_path = path;
	_reservedGazeX = _currentGazeX;
	_reservedGazeY = _currentGazeY;
	_partial = partial;
}

void ScreenshotHandler::TakeScreenshot(std::string fileName)
{
	// if no fileName is provided use timestamp
	if (fileName.length() < 1)
	{
		// Current timestamp to string
		time_t rawtime;
		struct tm * timeinfo;
		char buffer[80];
		time(&rawtime);

		timeinfo = localtime(&rawtime);
		strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S", timeinfo);
		fileName = buffer;
	}

	// Path
	std::string filePath = _path + fileName + ".bmp";
	if (!fs::exists(fs::path(_path))) {
		fs::create_directory(fs::path(_path));
	}

	// Prepare the coordinates
	int left = 0;
	int right = _width;
	int top = 0;
	int bottom = _height;

	// partial screenshot
	if (_partial && _reservedGazeX != -1 && _reservedGazeY != -1) {
		// 0,0 is top right
		// Prepare gaze data
		left = (_reservedGazeX - setup::BMP_GAZE_RADIUS / 2);
		if (left < 0)
			left = 0;

		right = _reservedGazeX + setup::BMP_GAZE_RADIUS / 2;
		if (right > _width)
			right = _width;

		top = _reservedGazeY - setup::BMP_GAZE_RADIUS / 2;
		if (top < 0)
			top = 0;
		
		bottom = (_reservedGazeY + setup::BMP_GAZE_RADIUS / 2);
		if (bottom > _height)
			bottom = _height;

		// reset reserved coordinates
		_reservedGazeX = -1;
		_reservedGazeY = -1;
	}

	int outWidth = right - left;
	int outHeight = bottom - top;

	// Prepare Header
	unsigned char padding[3] = { 0, 0, 0 };
	int paddingSize = (4 - (outWidth*_bytesPerPixel) % 4) % 4;
	int filesize = 54 + 4 * outWidth*outHeight;
	unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
	unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 32,0 };

	bmpfileheader[2] = (unsigned char)(filesize);
	bmpfileheader[3] = (unsigned char)(filesize >> 8);
	bmpfileheader[4] = (unsigned char)(filesize >> 16);
	bmpfileheader[5] = (unsigned char)(filesize >> 24);

	bmpinfoheader[4] = (unsigned char)(outWidth);
	bmpinfoheader[5] = (unsigned char)(outWidth >> 8);
	bmpinfoheader[6] = (unsigned char)(outWidth >> 16);
	bmpinfoheader[7] = (unsigned char)(outWidth >> 24);
	bmpinfoheader[8] = (unsigned char)(outHeight);
	bmpinfoheader[9] = (unsigned char)(outHeight >> 8);
	bmpinfoheader[10] = (unsigned char)(outHeight >> 16);
	bmpinfoheader[11] = (unsigned char)(outHeight >> 24);

	// Begin writing to File
	FILE* imageFile = fopen(filePath.c_str(), "wb");

	// Write Header
	fwrite(bmpfileheader, 1, 14, imageFile);
	fwrite(bmpinfoheader, 1, 40, imageFile);

	for (int h = bottom - 1; h >= top; h--) {
		fwrite(_buffer + (h*_width + left) * _bytesPerPixel, _bytesPerPixel, outWidth, imageFile);
		fwrite(padding, 1, paddingSize, imageFile);
	}

	// End writing to File
	fclose(imageFile);

	LogInfo("ScreenshotHandler: Saved: \"", fileName, ".bmp\"");
};
