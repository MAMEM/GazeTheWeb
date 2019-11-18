//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================

#pragma once
#include <string>

class ScreenshotHandler {

public:

	// Get instance
	static ScreenshotHandler& instance()
	{
		static ScreenshotHandler _instance;
		return _instance;
	}

	// Destructor
	~ScreenshotHandler() {};

	// Store the current rendered screen
	void SetBuffer(unsigned char const * buffer, int width, int height, int bytesPerPixel);

	// Store the current gaze
	void SetGaze(int x, int y);

	// Takes screenshot, saves it in "path", filename is a timestamp (%Y-%m-%d-%H-%M-%S)
	void TakeScreenshot(bool partial, std::string path);

	// Takes screenshot, saves it in "path", with given filename
	void TakeScreenshot(bool partial, std::string path, std::string fileName);

private:

	// Store Buffer after rendering, so you can take the Screenshot everytime you want to
	const unsigned char* _buffer = (unsigned char const*)0;
	int _width = 0;
	int _height = 0;
	int _bytesPerPixel = 0;

	// Gaze Coordinates
	int _gazeX = -1;
	int _gazeY = -1;
};
