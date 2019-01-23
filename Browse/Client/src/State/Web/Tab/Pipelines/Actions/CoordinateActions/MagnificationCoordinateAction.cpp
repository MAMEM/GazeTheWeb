//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "MagnificationCoordinateAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "src/Setup.h"
#include "submodules/glm/glm/gtx/vector_angle.hpp"
#include <algorithm>

MagnificationCoordinateAction::MagnificationCoordinateAction(TabInteractionInterface* pTab, bool doDimming) : Action(pTab)
{
	// Save members
	_doDimming = doDimming;

    // Add in- and output data slots
    AddVec2OutputSlot("coordinate");
}

bool MagnificationCoordinateAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject)
{
	// Function transforms coordinate from relative WebView coordinates to CEFPixel coordinates on page
	const std::function<void(const float&, const glm::vec2&, const glm::vec2&, glm::vec2&)> pageCoordinate
		= [&](const float& rZoom, const glm::vec2& rRelativeMagnificationCoordinate, const glm::vec2& rRelativeCenterOffset, glm::vec2& rCoordinate)
	{
		// Analogous to shader in WebView
		rCoordinate += rRelativeCenterOffset; // add center offset
		rCoordinate -= rRelativeMagnificationCoordinate; // move magnification coordinate to origin
		rCoordinate *= rZoom; // apply zoom
		rCoordinate += rRelativeMagnificationCoordinate; // move back
		rCoordinate *= glm::vec2(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY()); // bring into pixel space of CEF
	};

	// Function to apply eyeGUI gaze drift correction to WebView relative coordinate
	const std::function<void(glm::vec2&)> driftCorrection = [&](glm::vec2& rCoordinate)
	{
		// From relative WebView gaze to window pixel coordinate
		float windowPixelX = (rCoordinate.x * (float)_pTab->GetWebViewWidth()) + (float)_pTab->GetWebViewX();
		float windowPixelY = (rCoordinate.y * (float)_pTab->GetWebViewHeight()) + (float)_pTab->GetWebViewY();

		// Apply drift correction
		_pTab->ApplyGazeDriftCorrection(windowPixelX, windowPixelY);

		// Back from window pixel coordinate to relative WebView gaze
		rCoordinate.x = (windowPixelX - (float)_pTab->GetWebViewX()) / (float)_pTab->GetWebViewWidth();
		rCoordinate.y = (windowPixelY - (float)_pTab->GetWebViewY()) / (float)_pTab->GetWebViewHeight();
	};

	// Update magnification
	if (_magnify && _magnification < 1.f)
	{
		_magnification += tpf / MAGNIFICATION_ANIMATION_DURATION;
		_magnification = glm::min(_magnification, 1.f);
	}
	float magnificationAnimation = 1.f - (glm::cos(glm::pi<float>()*_magnification) + 1.f) / 2.f;

	// Current gaze
	glm::vec2 relativeGazeCoordinate = glm::vec2(spInput->webViewRelativeGazeX, spInput->webViewRelativeGazeY); // relative WebView space

	// Values of interest
	glm::vec2 relativeCenterOffset = magnificationAnimation * (_relativeMagnificationCenter - glm::vec2(0.5f, 0.5f));
	float zoom = (magnificationAnimation * MAGNIFICATION) + (1.f - magnificationAnimation);
	glm::vec2 relativeMagnificationCenter = _relativeMagnificationCenter;

	// Decrease fixationWaitTime
	if (fixationWaitTime > 0)
	{
		fixationWaitTime -= tpf;
		glm::max(fixationWaitTime, 0.f);
	}

	// Decide whether to magnify or to finish
	bool finished = false;
	if (!spInput->gazeUponGUI && (spInput->instantInteraction || (!spInput->gazeEmulated && fixationWaitTime <= 0 && spInput->fixationDuration >= FIXATION_DURATION))) // user demands on instant interaction or fixates on the screen
	{
		// Check for magnification
		if (_magnification >= 1.f) // already magnified, so finish this action
		{
			// Set output
			glm::vec2 coordinate = relativeGazeCoordinate;

			// Drift correction
			driftCorrection(coordinate);

			// Further transformation to CEF pixel space
			pageCoordinate(zoom, relativeMagnificationCenter, relativeCenterOffset, coordinate); // transform gaze relative to WebView to page coordinates
			SetOutputValue("coordinate", coordinate); // into pixel space of CEF

			// Finish this action
			finished = true;
		}
		else if(_magnification <= 0.f) // not yet magnified, do it now
		{
			// Set magnification center
			_relativeMagnificationCenter = relativeGazeCoordinate;

			// Drift correction
			driftCorrection(_relativeMagnificationCenter);

			// Remember magnification
			_magnify = true;

			// Reset fixation wait time so no accidential instant interaction after magnification can happen
			fixationWaitTime = FIXATION_DURATION;
		}
		// else: in the middle of magnification animation
	}

	// Decrement dimming
	_dimming += tpf;
	_dimming = glm::min(_dimming, DIMMING_DURATION);

	// Tell WebView about zoom
	WebViewParameters webViewParameters;
	webViewParameters.centerOffset = relativeCenterOffset;
	webViewParameters.zoom = zoom;
	webViewParameters.zoomPosition = relativeMagnificationCenter;
	if (_doDimming) { webViewParameters.dim = DIMMING_VALUE * (_dimming / DIMMING_DURATION); }
	_pTab->SetWebViewParameters(webViewParameters);

    // Return whether finished
    return finished;
}

void MagnificationCoordinateAction::Draw() const
{

}

void MagnificationCoordinateAction::Activate()
{

}

void MagnificationCoordinateAction::Deactivate()
{
	// Reset WebView (necessary because of dimming)
	WebViewParameters webViewParameters;
	_pTab->SetWebViewParameters(webViewParameters);
}

void MagnificationCoordinateAction::Abort()
{

}
