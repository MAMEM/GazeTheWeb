//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Input struct. Origin in upper left display corner.

#ifndef INPUT_H_
#define INPUT_H_

#include <memory>

class Input
{
public:

    Input(
		bool windowFocused,
		float gazeX,
		float gazeY,
		float rawGazeX,
		float rawGazeY,
		double gazeAge,
		bool gazeEmulated,
		bool gazeUponGUI,
		bool instantInteraction,
		float fixationDuration) :
	windowFocused(windowFocused),
	gazeX(gazeX),
	gazeY(gazeY),
	rawGazeX(rawGazeX),
	rawGazeY(rawGazeY),
	gazeAge(gazeAge),
	gazeEmulated(gazeEmulated),
	gazeUponGUI(gazeUponGUI),
	instantInteraction(instantInteraction),
	fixationDuration(fixationDuration) {}

	// Fields
	bool windowFocused;
    float gazeX;
	float gazeY;
	float rawGazeX;
	float rawGazeY;
	double gazeAge;
	bool gazeEmulated;
    bool gazeUponGUI;
	bool instantInteraction;
	float fixationDuration; // duration of current fixation (zero if currently saccade happening)
};

class TabInput
{
public:
	TabInput(
		const std::shared_ptr<const Input> spInput,
		int webViewX,
		int webViewY,
		int webViewWidth,
		int webViewHeight,
		int webViewResolutionX,
		int webViewResolutionY) :

		// Fields from input
		windowFocused(spInput->windowFocused),
		gazeX(spInput->gazeX),
		gazeY(spInput->gazeY),
		rawGazeX(spInput->rawGazeX),
		rawGazeY(spInput->rawGazeY),
		gazeAge(spInput->gazeAge),
		gazeEmulated(spInput->gazeEmulated),
		gazeUponGUI(spInput->gazeUponGUI),
		instantInteraction(spInput->instantInteraction),
		fixationDuration(spInput->fixationDuration),

		// TabInput fields
		webViewPixelGazeX(spInput->gazeX - (float)webViewX),
		webViewPixelGazeY(spInput->gazeY - (float)webViewY),
		webViewPixelRawGazeX(spInput->rawGazeX - (float)webViewX),
		webViewPixelRawGazeY(spInput->rawGazeY - (float)webViewY),
		webViewRelativeGazeX(webViewPixelGazeX / (float)webViewWidth),
		webViewRelativeGazeY(webViewPixelGazeY / (float)webViewHeight),
		webViewRelativeRawGazeX(webViewPixelRawGazeX / (float)webViewWidth),
		webViewRelativeRawGazeY(webViewPixelRawGazeY / (float)webViewHeight),
		CEFPixelGazeX(webViewRelativeGazeX * (float)webViewResolutionX),
		CEFPixelGazeY(webViewRelativeGazeY * (float)webViewResolutionY),
		CEFPixelRawGazeX(webViewRelativeRawGazeX * (float)webViewResolutionX),
		CEFPixelRawGazeY(webViewRelativeRawGazeY * (float)webViewResolutionY),
		insideWebView(
			webViewRelativeGazeX < 1.f
			&& webViewRelativeGazeX >= 0
			&& webViewRelativeGazeY < 1.f
			&& webViewRelativeGazeY >= 0)
		{}

	// Fields
	float webViewPixelGazeX;
	float webViewPixelGazeY;
	float webViewPixelRawGazeX;
	float webViewPixelRawGazeY;
	float webViewRelativeGazeX;
	float webViewRelativeGazeY;
	float webViewRelativeRawGazeX;
	float webViewRelativeRawGazeY;
	float CEFPixelGazeX;
	float CEFPixelGazeY;
	float CEFPixelRawGazeX;
	float CEFPixelRawGazeY;
	bool insideWebView;

	// Fields from input as reference
	const bool& windowFocused;
	const float& gazeX;
	const float& gazeY;
	const float& rawGazeX;
	const float& rawGazeY;
	const double& gazeAge;
	const bool& gazeEmulated;
	const bool& gazeUponGUI;
	const bool& instantInteraction;
	const float& fixationDuration;
};

#endif // INPUT_H_
