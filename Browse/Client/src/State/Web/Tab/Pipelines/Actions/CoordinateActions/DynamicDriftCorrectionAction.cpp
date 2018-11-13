//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "DynamicDriftCorrectionAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "src/Setup.h"
#include "submodules/glm/glm/gtx/vector_angle.hpp"
#include "submodules/glm/glm/gtx/component_wise.hpp"
#include <algorithm>

DynamicDriftCorrectionAction::DynamicDriftCorrectionAction(TabInteractionInterface* pTab, bool doDimming) : Action(pTab)
{
	// Save members
	_doDimming = doDimming;

    // Add in- and output data slots
    AddVec2OutputSlot("coordinate");
}

bool DynamicDriftCorrectionAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput)
{
	// TODO
	// - incoporate raw gaze data

	// ### PREPARATION ###

	// Speed of zooming
	float zoomSpeed = 0.f;

	// Size of page in CEF pixels
	const glm::vec2 cefPixels(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY());

	// Function transforms coordinate from relative WebView coordinates to CEFPixel coordinates on page
	const std::function<void(const float&, const glm::vec2&, const glm::vec2&, glm::vec2&)> pageCoordinate 
		= [&](const float& rLogZoom, const glm::vec2& rRelativeZoomCoordinate, const glm::vec2& rRelativeCenterOffset, glm::vec2& rCoordinate)
	{
		// Analogous to shader in WebView
		rCoordinate += rRelativeCenterOffset; // add center offset
		rCoordinate -= rRelativeZoomCoordinate; // move zoom coordinate to origin
		rCoordinate *= rLogZoom; // apply zoom
		rCoordinate += rRelativeZoomCoordinate; // move back
		rCoordinate *= cefPixels; // bring into pixel space of CEF
	};

	// Function calling above function with current values
	const std::function<void(glm::vec2&)> currentPageCoordinate
		= [&](glm::vec2& rCoordinate)
	{
		pageCoordinate(_logZoom, _relativeZoomCoordinate, _relativeCenterOffset, rCoordinate);
	};

	// Current gaze
	glm::vec2 relativeGazeCoordinate = glm::vec2(spInput->webViewRelativeGazeX, spInput->webViewRelativeGazeY); // relative WebView space

	// ### UPDATE ZOOM SPEED, ZOOM CENTER AND CENTER OFFSET ###

	// Only allow zoom in when gaze upon WebView and not yet used
	if (spInput->insideWebView && !spInput->gazeUponGUI) // TODO: gazeUsed really good idea here? Maybe later null pointer?
	{
		switch (_state)
		{
		case State::ZOOM:
		{
			// Update deviation value (fade away deviation)
			_deviation = glm::max(0.f, _deviation - (tpf / DEVIATION_FADING_DURATION));

			// Update zoom coordinate
			if (!_firstUpdate)
			{
				// Caculate delta on page for deviation
				glm::vec2 pixelGazeCoordinate = relativeGazeCoordinate;
				currentPageCoordinate(pixelGazeCoordinate);
				glm::vec2 pixelZoomCoordinate = _relativeZoomCoordinate * cefPixels;
				const float pixelDelta = glm::distance(pixelGazeCoordinate, pixelZoomCoordinate);

				// Set length of delta to deviation if bigger than current deviation
				_deviation = glm::min(1.f, glm::max(pixelDelta / glm::compMax(cefPixels), _deviation));

				// Move zoom coordinate towards new coordinate
				const glm::vec2 relativeDelta =
					(relativeGazeCoordinate + _relativeCenterOffset) // visually, the zoom coordinate is moved by relative center offset. So adapt input to this
					- _relativeZoomCoordinate;
				_relativeZoomCoordinate += relativeDelta * glm::min(1.f, (tpf / MOVE_DURATION));

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
			const glm::vec2 clampedRelativeZoom = glm::clamp(_relativeZoomCoordinate, glm::vec2(0.f), glm::vec2(1.f)); // clamp within page for determining relative center offset
			const float zoomWeight = ((1.f - _logZoom) / (1.f - MAX_ORIENTATION_LOG_ZOOM)); // projects zoom level to [0..1]
			_relativeCenterOffset =
				CENTER_OFFSET_MULTIPLIER
				* zoomWeight // weight with zoom (starting at zero) to have more centered version at higher zoom level
				* (clampedRelativeZoom - 0.5f); // vector from WebView center to current zoom coordinate

			// Get out of case
			break;
		}
		case State::DEBUG:
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

	// ### UPDATE ON OUTPUT VALUE ###

	// Decide whether zooming is finished
	bool finished = false;

	// Instant interaction handling
	if (spInput->insideWebView && !spInput->gazeUponGUI && spInput->instantInteraction) // user demands on instant interaction
	{
		// Calculate pixel gaze coordiante on page
		glm::vec2 pixelGazeCoordinate = relativeGazeCoordinate;
		pageCoordinate(_logZoom, _relativeZoomCoordinate, _relativeCenterOffset, pixelGazeCoordinate); // CEFPixel space

		// Set coordinate in output value. Use current gaze position
		SetOutputValue("coordinate", pixelGazeCoordinate);

		// Return success
		finished = true;
	}

	// Update samples
	std::for_each(_sampleData.begin(), _sampleData.end(), [&](SampleData& rSampleData) { rSampleData.lifetime -= tpf; });
	_sampleData.erase(
		std::remove_if(
			_sampleData.begin(),
			_sampleData.end(),
			[&](const SampleData& rSampleData) { return rSampleData.lifetime <= 0.f; }),
		_sampleData.end());

	// Add new sample
	_sampleData.push_back(SampleData(_logZoom, relativeGazeCoordinate, _relativeZoomCoordinate, _relativeCenterOffset));
	
	// Proceed depending on the state
	switch (_state)
	{
		case State::ZOOM:
		{
			// TODO: Reintegrate center offset and filter multiple sample data sets
			// -> All values should be in page coordinates (so relative to page, in pixels)

			// TODO: limit zooming, maybe define maximum zoom level (something bigger than zero)

			if (_logZoom < 0.75f) // wait until some samples exist
			{
				// Choose sample
				SampleData sample = _sampleData.front();

				// Determine movement of zoom coordinate between current and sample
				glm::vec2 zoomCoordinateDeltaVector = (_relativeZoomCoordinate - sample.relativeZoomCoordinate) * cefPixels;
				float zoomCoordinateDelta = glm::length(zoomCoordinateDeltaVector);

				// Current pixel gaze coordinate on page with values as sample was taken
				glm::vec2 pixelGazeCoordinate = relativeGazeCoordinate;
				pageCoordinate(sample.logZoom, sample.relativeZoomCoordinate, sample.relativeCenterOffset, pixelGazeCoordinate);

				// Pixel gaze coordinate on page at time where sample has been taken
				glm::vec2 samplePixelGazeCoordinate = sample.relativeGazeCoordinate;
				pageCoordinate(_logZoom, _relativeZoomCoordinate, _relativeCenterOffset, samplePixelGazeCoordinate);

				// Delta of gaze
				glm::vec2 gazeDeltaVector = pixelGazeCoordinate - samplePixelGazeCoordinate;
				float gazeDelta = glm::length(gazeDeltaVector);

				// Angle between zoomCoordinateDeltaVector and gazeDeltaVector
				float deltaAngle = glm::degrees(glm::angle(glm::normalize(gazeDeltaVector), glm::normalize(zoomCoordinateDeltaVector)));
				LogInfo("DriftAngle: ", deltaAngle); // TODO: which value interval is the angle in?

				// Decide to go directly for zoom coordinate (good calibration) or drift corrected coordinate (poor calibration) or continue zooming
				if (zoomCoordinateDelta < 1.f) // zoom coordinate has not changed in pixels on page
				{
					SetOutputValue("coordinate", _relativeZoomCoordinate * cefPixels);
					// finished = true; // TODO debugging
					_state = State::DEBUG;
					LogInfo("Zoom coordinate used");
				}
				else if (deltaAngle <= 5) // angle of gazing is rather static TODO: should the delta be of some minimal size?
				{
					// Inverse zooms
					float zoom = 1.f / _logZoom;
					float sampleZoom = 1.f / sample.logZoom;

					// Radius where fixated coordinate lies
					float radius = gazeDelta - zoomCoordinateDelta + (zoom * zoomCoordinateDelta);
					radius /= zoom - sampleZoom;

					// Page coordinat of relative zoom coordinate
					glm::vec2 samplePixelZoomCoordinate = sample.relativeZoomCoordinate * glm::vec2(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY());

					// Actual fixation point
					glm::vec2 fixation = (glm::normalize(zoomCoordinateDeltaVector) * radius) + samplePixelZoomCoordinate;
					SetOutputValue("coordinate", fixation);

					// finished = true; // TODO debugging
					_state = State::DEBUG;
					LogInfo("Drift corrected coordinate used");
				}
				
				// else continue
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

void DynamicDriftCorrectionAction::Draw() const
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

		/*
		// Zoom coordinate
		glm::vec2 zoomCoordinate(_relativeZoomCoordinate);
		applyZooming(zoomCoordinate);
		_pTab->Debug_DrawRectangle(zoomCoordinate, glm::vec2(5, 5), glm::vec3(1, 0, 0));
		*/

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

void DynamicDriftCorrectionAction::Activate()
{
	// Nothing to do
}

void DynamicDriftCorrectionAction::Deactivate()
{
	// Reset WebView (necessary because of dimming)
	WebViewParameters webViewParameters;
	_pTab->SetWebViewParameters(webViewParameters);
}

void DynamicDriftCorrectionAction::Abort()
{

}
