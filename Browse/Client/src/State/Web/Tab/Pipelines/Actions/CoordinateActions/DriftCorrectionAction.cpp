//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "DriftCorrectionAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "src/Setup.h"
#include "submodules/glm/glm/gtx/vector_angle.hpp"
#include "submodules/glm/glm/gtx/component_wise.hpp"
#include <algorithm>

DriftCorrectionAction::DriftCorrectionAction(TabInteractionInterface* pTab, bool doDimming) : Action(pTab)
{
	// Save members
	_doDimming = doDimming;

    // Add in- and output data slots
    AddVec2OutputSlot("coordinate");
}

bool DriftCorrectionAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject)
{
	// ### PREPARATION ###

	// Speed of zooming
	float zoomSpeed = 0.f;

	// Function transforms coordinate from relative WebView coordinates to CEFPixel coordinates on page
	const std::function<void(const float&, const glm::vec2&, const glm::vec2&, glm::vec2&)> pageCoordinate 
		= [&](const float& rLogZoom, const glm::vec2& rRelativeZoomCoordinate, const glm::vec2& rRelativeCenterOffset, glm::vec2& rCoordinate)
	{
		// Analogous to shader in WebView
		rCoordinate += rRelativeCenterOffset; // add center offset
		rCoordinate -= rRelativeZoomCoordinate; // move zoom coordinate to origin
		rCoordinate *= rLogZoom; // apply zoom
		rCoordinate += rRelativeZoomCoordinate; // move back
		rCoordinate *= glm::vec2(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY()); // bring into pixel space of CEF
	};

	// Function calling above function with current values
	const std::function<void(glm::vec2&)> currentPageCoordinate
		= [&](glm::vec2& rCoordinate)
	{
		pageCoordinate(_logZoom, _relativeZoomCoordinate, _relativeCenterOffset, rCoordinate);
	};

	// Current raw! gaze (filtered here manually)
	glm::vec2 relativeGazeCoordinate = glm::vec2(spInput->webViewRelativeRawGazeX, spInput->webViewRelativeRawGazeY); // relative WebView space

	// ### UPDATE ZOOM SPEED, ZOOM CENTER AND CENTER OFFSET ###

	// Only allow zoom in when gaze upon WebView and not yet used
	if (spInput->insideWebView && !spInput->gazeUponGUI)
	{
		switch (_state)
		{
		case State::ORIENTATE: // orientation phase
		{
			// Update deviation value (fade away deviation)
			_deviation = glm::max(0.f, _deviation - (tpf / DEVIATION_FADING_DURATION));

			// Update zoom coordinate
			if (!_firstUpdate)
			{
				// Caculate delta on page for deviation
				glm::vec2 pixelGazeCoordinate = relativeGazeCoordinate;
				currentPageCoordinate(pixelGazeCoordinate);
				glm::vec2 pixelZoomCoordinate = _relativeZoomCoordinate * glm::vec2(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY());
				float pixelDelta = glm::distance(pixelGazeCoordinate, pixelZoomCoordinate);

				// Move zoom coordinate towards new coordinate
				glm::vec2 relativeDelta =
					(relativeGazeCoordinate + _relativeCenterOffset) // visually, the zoom coordinate is moved by relative center offset. So adapt input to this
					- _relativeZoomCoordinate;
				_relativeZoomCoordinate += relativeDelta * glm::min(1.f, (tpf / MOVE_DURATION));

				// Set length of delta to deviation if bigger than current deviation
				_deviation = glm::min(1.f, glm::max(pixelDelta / glm::max(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY()), _deviation));

				// If at the moment a high deviation is given, try to zoom out to give user more overview
				zoomSpeed = ZOOM_SPEED - glm::min(1.f, DEVIATION_WEIGHT * _deviation); // TODO weight deviation more intelligent
			}
			else // first frame of execution
			{
				// Use current gaze coordinate as new coordinate
				_relativeZoomCoordinate = relativeGazeCoordinate;

				// Since only for first frame, do not do it again
				_firstUpdate = false;
			}

			// Calculated center offset. This moves the WebView content towards the center for better gaze precision
			glm::vec2 clampedRelativeZoom = glm::clamp(_relativeZoomCoordinate, glm::vec2(0.f), glm::vec2(1.f)); // clamp within page for determining relative center offset
			float zoomWeight = ((1.f - _logZoom) / (1.f - MAX_ORIENTATION_LOG_ZOOM)); // projects zoom level to [0..1]
			_relativeCenterOffset =
				CENTER_OFFSET_MULTIPLIER
				* zoomWeight // weight with zoom (starting at zero) to have more centered version at higher zoom level
				* (clampedRelativeZoom - 0.5f); // vector from WebView center to current zoom coordinate

			// Get out of case
			break;
		}
		case State::ZOOM: // zoom phase
			zoomSpeed = 0.25f;
			break;
		default: // in fact: debug phase
			zoomSpeed = 0.f;
			break;
		}
	}

	// ### UPDATE ZOOM ###

	// Update linear zoom
	_linZoom += tpf * zoomSpeed; // frame rate depended? at least complex

	// Clamp linear zoom (one is standard, everything higher is zoomed)
	_linZoom = glm::max(_linZoom, 1.f);

	// Make zoom better with log function
	_logZoom = 1.f - glm::max(glm::log(_linZoom), 0.f); // log zooming is starting at one and getting smaller with higher _linZoom

	// ### UPDATE VALUES ###

	// Decide whether zooming is finished
	bool finished = false;

	// Instant interaction handling
	if (!spInput->gazeUponGUI && spInput->instantInteraction) // user demands on instant interaction
	{
		// Calculate pixel gaze coordiante on page
		glm::vec2 pixelGazeCoordinate = relativeGazeCoordinate; // CEFPixel space
		pageCoordinate(_logZoom, _relativeZoomCoordinate, _relativeCenterOffset, pixelGazeCoordinate);

		// Set coordinate in output value. Use current gaze position
		SetOutputValue("coordinate", pixelGazeCoordinate);

		// Return success
		finished = true;
	}

	// Proceed depending on the state
	switch (_state)
	{
		case State::ORIENTATE:
		{
			if (_logZoom <= MAX_ORIENTATION_LOG_ZOOM)
			{
				// Store sample from that time
				_sampleData.logZoom = _logZoom;
				_sampleData.relativeGazeCoordinate = relativeGazeCoordinate;
				_sampleData.relativeZoomCoordinate = _relativeZoomCoordinate;
				_sampleData.relativeCenterOffset = _relativeCenterOffset;

				// End orientation
				_state = State::ZOOM;
			}
			break;
		}
		case State::ZOOM:
		{
			if (_logZoom <= MAX_DRIFT_CORRECTION_LOG_ZOOM)
			{
				_state = State::WAIT;
			}
			break;
		}
		case State::WAIT:
		{
			_gazeCalmDownTime -= tpf;
			if (_gazeCalmDownTime < 0)
			{
				// TODO check whether drift is significant or can be ignored (for very good calibration)

				// Use filtered gaze here, not raw one (TODO: fixation based filtering might be a bad idea, as here something moving is fixated and not a static position)
				glm::vec2 relativeFilteredGazeCoordinate = glm::vec2(spInput->webViewRelativeGazeX, spInput->webViewRelativeGazeY); // relative WebView space

				// Current pixel gaze coordinate on page with values as sample was taken
				glm::vec2 pixelFilteredGazeCoordinate = relativeFilteredGazeCoordinate;
				pageCoordinate(_sampleData.logZoom, _sampleData.relativeZoomCoordinate, _sampleData.relativeCenterOffset, pixelFilteredGazeCoordinate);

				// Sample pixel gaze coordinate on page
				glm::vec2 samplePixelGazeCoordinate = _sampleData.relativeGazeCoordinate;
				pageCoordinate(_logZoom, _sampleData.relativeZoomCoordinate, _sampleData.relativeCenterOffset, samplePixelGazeCoordinate); // sampleData zoom coordinate and current should be the same

				// Page coordinate of relative zoom coordinate
				glm::vec2 samplePixelZoomCoordinate = _sampleData.relativeZoomCoordinate * glm::vec2(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY());

				// Calculate drift corrected fixation coordinate
				glm::vec2 drift = pixelFilteredGazeCoordinate - samplePixelGazeCoordinate;
				float radius = glm::length(drift) / ((1.f/_logZoom) - (1.f/_sampleData.logZoom));
				glm::vec2 fixation = (glm::normalize(drift) * radius) + samplePixelZoomCoordinate;
				SetOutputValue("coordinate", fixation);

				// Return success
				finished = true;
				// _state = State::DEBUG; // TODO debugging
			}
			break;
		}
		case State::DEBUG:
		{
			_logZoom = 1.f;
			_relativeCenterOffset = glm::vec2(0, 0);
			break;
		}
	}

	// ### UPDATE WEBVIEW ###

	// Decrement dimming
	_dimming += tpf;
	_dimming = glm::min(_dimming, DIMMING_DURATION);

	// Tell WebView about zoom and dimming
	WebViewParameters webViewParameters;
	webViewParameters.centerOffset = _relativeCenterOffset;
	webViewParameters.zoom = _logZoom;
	webViewParameters.zoomPosition = _relativeZoomCoordinate;
	if (_doDimming) { webViewParameters.dim = DIMMING_VALUE * (_dimming / DIMMING_DURATION); }
	_pTab->SetWebViewParameters(webViewParameters);

    // Return whether finished
    return finished;
}

void DriftCorrectionAction::Draw() const
{
	// Do draw some stuff for debugging
#ifdef CLIENT_DEBUG
		// WebView pixels
		glm::vec2 webViewPixels(_pTab->GetWebViewWidth(), _pTab->GetWebViewHeight());

		// Function to move coordinate according to current zoom. Takes relative WebView coordinate
		const std::function<void(glm::vec2&)> applyZooming = [&](glm::vec2& rCoordinate)
		{
			rCoordinate -= _relativeZoomCoordinate;
			rCoordinate /= _logZoom; // inverse to WebView shader
			rCoordinate += _relativeZoomCoordinate;
			rCoordinate -= _relativeCenterOffset; // inverse to WebView shader
			rCoordinate *= webViewPixels;
		};

		// Zoom coordinate
		glm::vec2 zoomCoordinate(_relativeZoomCoordinate);
		applyZooming(zoomCoordinate);
		_pTab->Debug_DrawRectangle(zoomCoordinate, glm::vec2(5, 5), glm::vec3(1, 0, 0));

		// Click coordinate
		glm::vec2 coordinate;
		if (GetOutputValue("coordinate", coordinate)) // only show when set
		{
			// TODO: convert from CEF Pixel space to WebView Pixel space
			_pTab->Debug_DrawRectangle(coordinate, glm::vec2(5, 5), glm::vec3(0, 1, 0));
		}

		// Testing visualization
		glm::vec2 testCoordinate(0.3f, 0.5f);
		applyZooming(testCoordinate);
		_pTab->Debug_DrawRectangle(testCoordinate, glm::vec2(5, 5), glm::vec3(0, 0, 1));
#endif // CLIENT_DEBUG
}

void DriftCorrectionAction::Activate()
{
	// Nothing to do
}

void DriftCorrectionAction::Deactivate()
{
	// Reset WebView (necessary because of dimming)
	WebViewParameters webViewParameters;
	_pTab->SetWebViewParameters(webViewParameters);
}

void DriftCorrectionAction::Abort()
{

}
