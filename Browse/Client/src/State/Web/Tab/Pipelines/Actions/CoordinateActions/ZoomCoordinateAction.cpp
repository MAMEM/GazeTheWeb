//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "ZoomCoordinateAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "submodules/glm/glm/gtx/vector_angle.hpp"
#include <algorithm>

ZoomCoordinateAction::ZoomCoordinateAction(TabInteractionInterface* pTab, bool doDimming) : Action(pTab)
{
	// Save members
	_doDimming = doDimming;

    // Add in- and output data slots
    AddVec2OutputSlot("coordinate");
}

bool ZoomCoordinateAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput)
{
	// Speed of zooming
	float zoomSpeed = 0.f;

	// Web view resolution
	const glm::vec2 webViewResolution(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY());

	// Function transforms coordinate from relative WebView coordinates to CEFPixel coordinates on page
	const std::function<void(const float&, const glm::vec2&, const glm::vec2&, glm::vec2&)> pageCoordinate
		= [&](const float& rZoom, const glm::vec2& rRelativeZoomCoordinate, const glm::vec2& rRelativeCenterOffset, glm::vec2& rCoordinate)
	{
		// Analogous to shader in WebView
		rCoordinate += rRelativeCenterOffset; // add center offset
		rCoordinate -= rRelativeZoomCoordinate; // move zoom coordinate to origin
		rCoordinate *= rZoom; // apply zoom
		rCoordinate += rRelativeZoomCoordinate; // move back
		rCoordinate *= webViewResolution; // bring into pixel space of CEF
	};

	// Function calling above function with current values
	const std::function<void(glm::vec2&)> currentPageCoordinate
		= [&](glm::vec2& rCoordinate)
	{
		pageCoordinate(_zoom, _relativeZoomCoordinate, _relativeCenterOffset, rCoordinate);
	};

	// Current raw! gaze (filtered here through zoom coordinate calculation)
	glm::vec2 relativeGazeCoordinate(spInput->webViewRelativeRawGazeX, spInput->webViewRelativeRawGazeY); // relative WebView space

	// Current gaze in page pixel coordinates
	glm::vec2 pixelGazeCoordinate = relativeGazeCoordinate;
	currentPageCoordinate(pixelGazeCoordinate);

	// Only allow zoom in when gaze upon web view
	if (!spInput->gazeUponGUI && spInput->insideWebView)
	{
		// Update deviation value (fade away deviation)
		_deviation = glm::max(0.f, _deviation - (tpf / DEVIATION_FADING_DURATION));

		// Update coordinate
		if (!_firstUpdate) // normal update
		{
			// Caculate offset of gaze on page in comparison to zoom coordinate
			glm::vec2 pixelZoomCoordinate = _relativeZoomCoordinate * webViewResolution;
			glm::vec2 pixelOffset = pixelGazeCoordinate - pixelZoomCoordinate;

			// Update zoom coordinate
			_relativeZoomCoordinate += (pixelOffset / webViewResolution) * glm::min(1.f, (tpf / MOVE_DURATION));

			// Set normalized offset as deviation if bigger than current deviation
			float normOffset = glm::length(pixelOffset / glm::max(webViewResolution.x, webViewResolution.y));
			_deviation = glm::min(1.f, glm::max(normOffset, _deviation)); // limit to one

			// If at the moment a high deviation is given, try to zoom out to give user more overview
			zoomSpeed = ZOOM_SPEED - glm::min(1.f, DEVIATION_WEIGHT * _deviation); // TODO weight deviation more intelligent
		}
		else // first frame of execution
		{
			// Use raw coordinate as new coordinate
			_relativeZoomCoordinate = relativeGazeCoordinate;

			// Since only for first frame, do not do it again
			_firstUpdate = false;
		}

		// Calculated center offset. This moves the WebView content towards the center for better gaze precision
		glm::vec2 clampedRelativeZoom = glm::clamp(_relativeZoomCoordinate, glm::vec2(0.f), glm::vec2(1.f)); // clamp within page for determining relative center offset
		float zoomWeight = ((1.f - _zoom) / (1.f - MAX_ZOOM)); // projects zoom level to [0..1]
		_relativeCenterOffset =
			CENTER_OFFSET_MULTIPLIER
			* zoomWeight // weight with zoom (starting at zero) to have more centered version at higher zoom level
			* (clampedRelativeZoom - 0.5f); // vector from WebView center to current zoom coordinate
	}

	// Update linear zoom
	_linZoom += tpf * zoomSpeed; // frame rate depended, because zoomSpeed is depending on deviation which is depending on tpf

	// Clamp linear zoom (zero is standard, everything higher is zoomed)
	_linZoom = glm::max(_linZoom, 0.f);

	// Make zoom better with exponential function and some delay for orientation
	_zoom = glm::min(1.f, glm::exp(-(_linZoom - ZOOM_DELAY)));

	// Decide whether zooming is finished
	bool finished = false;
	if (!spInput->gazeUponGUI && spInput->instantInteraction) // user demands on instant interaction
	{
		// Set coordinate in output value. Use current gaze position
		SetOutputValue("coordinate", pixelGazeCoordinate);

		// Return success
		finished = true;
	}
	else if ( // zooming is high enough for coordinate to be accurate
		_zoom <= MAX_ZOOM // just zoomed so far into that coordinate is just used
		|| ((_zoom <= 0.45f) && (_deviation < 0.01f))) // coordinate seems to be quite fixed, just do it
	{
		// Set coordinate in output value
        SetOutputValue("coordinate", glm::vec2(_relativeZoomCoordinate * webViewResolution)); // into pixel space of CEF

		// Return success
		finished = true;
	}

	// Decrement dimming
	_dimming += tpf;
	_dimming = glm::min(_dimming, DIMMING_DURATION);

	// Tell web view about zoom
	WebViewParameters webViewParameters;
	webViewParameters.centerOffset = _relativeCenterOffset;
	webViewParameters.zoom = _zoom;
	webViewParameters.zoomPosition = _relativeZoomCoordinate;
	if (_doDimming) { webViewParameters.dim = DIMMING_VALUE * (_dimming / DIMMING_DURATION); }
	_pTab->SetWebViewParameters(webViewParameters);

    // Return whether finished
    return finished;
}

void ZoomCoordinateAction::Draw() const
{

}

void ZoomCoordinateAction::Activate()
{

}

void ZoomCoordinateAction::Deactivate()
{
	// Reset web view (necessary because of dimming)
	WebViewParameters webViewParameters;
	_pTab->SetWebViewParameters(webViewParameters);
}

void ZoomCoordinateAction::Abort()
{

}
