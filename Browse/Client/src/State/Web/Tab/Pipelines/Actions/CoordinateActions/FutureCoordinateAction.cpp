//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "FutureCoordinateAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "submodules/glm/glm/gtx/vector_angle.hpp"
#include <algorithm>

// Deviation weight function
const std::function<float(float)> DEVIATION_WEIGHT = [](float dev) { return 10.f * dev; };

// Dimming duration
const float DIMMING_DURATION = 0.5f; // seconds until it is dimmed

// Dimming value
const float DIMMING_VALUE = 0.0f; // no dimming for experiment

// Deviation fading duration (how many seconds until full deviation is back to zero)
const float DEVIATION_FADING_DURATION = 0.5f;

// Multiplier of movement towards center (one means that on maximum zoom the outermost corner is moved into center)
const float CENTER_OFFSET_MULTIPLIER = 0.25f;

// Duration to replace current coordinate with input
const float MOVE_DURATION = 0.5f;

// Maximum zoom level
const float MAX_ZOOM = 0.1f;

// Maximum linear zoom duration
const float ZOOM_DURATION = 1.75f;

// Steepnes of zooming
const float ZOOM_STEEPNESS = 1.25f;

// Name of custom transformation
const std::string TRANS_NAME = "FutureCoordinateAction";

// Count of fixations considered for final output
const int FIXATION_COUNT = 15;

// Expected precision of fixation
const float FIXATION_PIXEL_PRECISION = 2.5f;

FutureCoordinateAction::FutureCoordinateAction(TabInteractionInterface* pTab, bool doDimming) : Action(pTab)
{
	// Save members
	_doDimming = doDimming;

    // Add in- and output data slots
    AddVec2OutputSlot("coordinate");
}

bool FutureCoordinateAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput)
{
	// ###################
	// ### DEFINITIONS ###
	// ###################

	// WebView resolution
	const glm::vec2 cefPixels(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY());

	// WebView resolution
	const glm::vec2 webViewPixels(_pTab->GetWebViewWidth(), _pTab->GetWebViewHeight());

	// Function transforms coordinate from relative WebView coordinates to CEFPixel coordinates on page
	const std::function<void(const float&, const glm::vec2&, const glm::vec2&, glm::vec2&)> pageCoordinate
		= [&](const float& rZoom, const glm::vec2& rRelativeZoomCoordinate, const glm::vec2& rRelativeCenterOffset, glm::vec2& rCoordinate)
	{
		// Analogous to shader in WebView
		rCoordinate += rRelativeCenterOffset; // add center offset
		rCoordinate -= rRelativeZoomCoordinate; // move zoom coordinate to origin
		rCoordinate *= rZoom; // apply zoom
		rCoordinate += rRelativeZoomCoordinate; // move back
		rCoordinate *= cefPixels; // bring into pixel space of CEF
	};

	// Function calling above function with current values
	const std::function<void(glm::vec2&)> currentPageCoordinate
		= [&](glm::vec2& rCoordinate)
	{
		pageCoordinate(_zoom, _relativeZoomCoordinate, _relativeCenterOffset, rCoordinate);
	};

	// Function to take pixel page values and returns relative WebView coordinates
	const std::function<void(const float&, const glm::vec2&, const glm::vec2&, glm::vec2&)> webViewCoordinate
		= [&](const float& rZoom, const glm::vec2& rRelativeZoomCoordinate, const glm::vec2& rRelativeCenterOffset, glm::vec2& rCoordinate)
	{
		rCoordinate /= cefPixels;
		rCoordinate -= rRelativeZoomCoordinate;
		rCoordinate /= rZoom; // inverse to WebView shader
		rCoordinate += rRelativeZoomCoordinate;
		rCoordinate -= rRelativeCenterOffset; // inverse to WebView shader
	};

	// Function calling above function with current values
	const std::function<void(glm::vec2&)> currentWebViewCoordinate
		= [&](glm::vec2& rCoordinate)
	{
		webViewCoordinate(_zoom, _relativeZoomCoordinate, _relativeCenterOffset, rCoordinate);
	};

	// Current raw! gaze (filtered here through zoom coordinate calculation)
	glm::vec2 pixelGazeCoordinate(spInput->webViewRelativeRawGazeX, spInput->webViewRelativeRawGazeY);
	currentPageCoordinate(pixelGazeCoordinate);

	// ##############################
	// ### ZOOM COORDINATE UPDATE ###
	// ##############################

	// Speed of zooming
	float zoomSpeed = 0.f;

	// Only allow zoom in when gaze upon web view
	if (!spInput->gazeUponGUI && spInput->insideWebView)
	{
		// Update deviation value (fade away deviation)
		_deviation = glm::max(0.f, _deviation - (tpf / DEVIATION_FADING_DURATION));

		// Update coordinate
		if (!_firstUpdate) // normal update
		{
			// Caculate offset of gaze on page in comparison to zoom coordinate
			glm::vec2 pixelZoomCoordinate = _relativeZoomCoordinate * cefPixels;
			glm::vec2 pixelOffset = pixelGazeCoordinate - pixelZoomCoordinate;

			// Update zoom coordinate
			_relativeZoomCoordinate += (pixelOffset / cefPixels) * glm::min(1.f, (tpf / MOVE_DURATION));

			// Set normalized offset as deviation if bigger than current deviation
			float normOffset = glm::length(pixelOffset / glm::max(cefPixels.x, cefPixels.y));
			_deviation = glm::min(1.f, glm::max(normOffset, _deviation)); // limit to one

			// If at the moment a high deviation is given, try to zoom out to give user more overview
			zoomSpeed = (1.f / ZOOM_DURATION) - glm::min(1.f, DEVIATION_WEIGHT(_deviation));
		}
		else // first frame of execution
		{
			// Use raw coordinate as new coordinate
			_relativeZoomCoordinate = glm::vec2(spInput->webViewRelativeRawGazeX, spInput->webViewRelativeRawGazeY);

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

	// Exponential zoom
	_linZoom = glm::max(0.f, _linZoom);
	_zoom = glm::min(1.f, glm::exp(-(_linZoom - 0.1f)));

	// ########################
	// ### DRIFT CORRECTION ###
	// ########################

	// Update and clean samples
	std::for_each(_sampleData.begin(), _sampleData.end(), [&](SampleData& rSampleData) { rSampleData.lifetime -= tpf; }); // update remaining lifetime
	_sampleData.erase(
		std::remove_if(
			_sampleData.begin(),
			_sampleData.end(),
			[&](const SampleData& rSampleData)
	{
		return rSampleData.lifetime <= 0.f;
	}),
		_sampleData.end()); // clean samples

	// Use filtered gaze here (on page space)
	glm::vec2 filteredRelativeGazeCoordinate(_spTrans->GetFilteredGazeX(TRANS_NAME), _spTrans->GetFilteredGazeY(TRANS_NAME)); // CEF page pixels
	currentWebViewCoordinate(filteredRelativeGazeCoordinate); // relative WebView space

	// Add new sample storing current values
	_sampleData.push_back(SampleData(_zoom, filteredRelativeGazeCoordinate, _relativeZoomCoordinate, _relativeCenterOffset));

	// Decide whether zooming is finished
	bool finished = false;
	SampleData sample = _sampleData.front(); // only use front of samples
	if (_zoom < 0.65f && _zoom < sample.zoom) // only proceed if current zoom is smaller than the on from the sample, so zoomed more
	{
		// ### WebView pixels

		// Current gaze in WebView pixels
		glm::vec2 pixelGaze = filteredRelativeGazeCoordinate * webViewPixels; // on screen

		// Sample gaze in WebView pixels
		glm::vec2 sampleRelativeGaze = sample.relativeGazeCoordinate;
		glm::vec2 samplePixelGaze = sampleRelativeGaze * webViewPixels; // on screen

		// Gaze delta between both gaze coordinates on screen
		glm::vec2 pixelGazeDelta = pixelGaze - samplePixelGaze; // on screen delta percepted by eye tracker

		// Center delta on screen
		glm::vec2 zoomCoordinate = _relativeZoomCoordinate * cefPixels; // CEF page pixels
		webViewCoordinate(_zoom, _relativeZoomCoordinate, _relativeCenterOffset, zoomCoordinate); // relative WebView
		zoomCoordinate *= webViewPixels; // WebView pixels
		glm::vec2 sampleZoomCoordinate = sample.relativeZoomCoordinate * cefPixels; // CEF page pixels
		webViewCoordinate(sample.zoom, sample.relativeZoomCoordinate, sample.relativeCenterOffset, sampleZoomCoordinate); // relative WebView
		sampleZoomCoordinate *= webViewPixels; // WebView pixels
		glm::vec2 pixelZoomCoordinateDelta = zoomCoordinate - sampleZoomCoordinate; // on screen delta

		// Remove offset introduced by center offset
		glm::vec2 pixelCenterOffsetDelta = (_relativeCenterOffset - sample.relativeCenterOffset) * webViewPixels; // screen offset caused by center offset application
		pixelGazeDelta += pixelCenterOffsetDelta;
		pixelZoomCoordinateDelta += pixelCenterOffsetDelta;

		// Bring pixel gaze delta from WebView pixels dimension to CEF webpage pixels dimension
		pixelGazeDelta = (pixelGazeDelta / webViewPixels) * cefPixels;
		pixelZoomCoordinateDelta = (pixelZoomCoordinateDelta / webViewPixels) * cefPixels;

		// ### CEF pixels

		// Radius in which fixated point on screen must have laid to cause such gaze delta through user tracking
		float z = glm::length(pixelZoomCoordinateDelta);
		float f = glm::length(pixelGazeDelta);
		float zoomA = 1.f / sample.zoom;
		float zoomB = 1.f / _zoom;
		float radius = (f - z + (zoomB * z)) / (zoomB - zoomA); // CEF pixels

		// Direction into which fixated point must have been moved
		glm::vec2 direction = glm::normalize(pixelGazeDelta);

		// Pixel position of zoom coordinate at sample record on page
		glm::vec2 samplePixelZoomCoordinate = sample.relativeZoomCoordinate * cefPixels;

		// Fixation in CEF pixels
		glm::vec2 pixelFixation = samplePixelZoomCoordinate + (direction * radius);

		// Output fixation
		LogInfo("Fixation: ", pixelFixation.x, ", ", pixelFixation.y);

		// Push back fixation
		_fixations.push_back(pixelFixation);
	}

	// Limit fixations
	int overlap = glm::max(0, (int)_fixations.size() - FIXATION_COUNT);
	for (int i = 0; i < overlap; i++)
	{
		_fixations.pop_front();
	}

	// Go over last fixations and calculate deviation
	const int size = _fixations.size();
	if (size >= FIXATION_COUNT)
	{
		// Calculate mean
		glm::vec2 mean(0, 0);
		for (const auto& rValue : _fixations)
		{
			mean += rValue;
		}
		mean /= (float)size;

		// Calculate mean distance to mean
		float meanDistance = 0.f;
		for (const auto& rValue : _fixations)
		{
			meanDistance += glm::distance(rValue, mean);
		}
		meanDistance /= (float)size;

		// Decide whether precision is high enough
		if (meanDistance < FIXATION_PIXEL_PRECISION)
		{
			// Fill output
			SetOutputValue("coordinate", mean);
			finished = true;
		}
	}

	// ############################
	// ### PAGE SPACE FILTERING ###
	// ############################

	// Update custom transformation
	double webViewX = _pTab->GetWebViewX();
	double webViewY = _pTab->GetWebViewY();
	double webViewWidth = _pTab->GetWebViewWidth();
	double webViewHeight = _pTab->GetWebViewHeight();
	auto zoom = _zoom;
	auto relativeZoomCoordinate = _relativeZoomCoordinate;
	auto relativeCenterOffset = _relativeCenterOffset;
	_spTrans->ChangeCustomTransformation(
		TRANS_NAME,
		[webViewX, webViewY, webViewWidth, webViewHeight, zoom, relativeZoomCoordinate, relativeCenterOffset, cefPixels](double& x, double& y)
	{
		// Bring raw gaze into relative space of WebView
		glm::vec2 gaze(x, y); // double 2D vector
		gaze.x -= webViewX;
		gaze.y -= webViewY;
		gaze.x /= webViewWidth;
		gaze.y /= webViewHeight;
		gaze = glm::clamp(gaze, glm::vec2(0, 0), glm::vec2(1, 1));

		// Now transform to pixel space of webpage
		gaze += relativeCenterOffset; // add center offset
		gaze -= relativeZoomCoordinate; // move zoom coordinate to origin
		gaze *= zoom; // apply zoom
		gaze += relativeZoomCoordinate; // move back
		gaze *= cefPixels; // bring into pixel space of CEF

		// Set references
		x = gaze.x;
		y = gaze.y;
	}
	);

	// #####################
	// ### VISUALIZATION ###
	// #####################	

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

	// ################
	// ### FALLBACK ###
	// ################

	// Finishing condition
	if (_zoom <= MAX_ZOOM)
	{
		finished = true;
	}

	// If finished but no coordinate determined, use zoom coordinate as fallback
	if(finished)
	{
		// Check whether coordinate was set
		glm::vec2 helper;
		if (!this->GetOutputValue("coordinate", helper))
		{
			SetOutputValue("coordinate", glm::vec2(_relativeZoomCoordinate * webViewPixels)); // into pixel space of CEF
		}
	}

    // Return whether finished
    return finished;
}

void FutureCoordinateAction::Draw() const
{
	// Do draw some stuff for debugging
#ifdef CLIENT_DEBUG
	// WebView pixels
	glm::vec2 webViewPixels(_pTab->GetWebViewWidth(), _pTab->GetWebViewHeight());

	// Function to move coordinate according to current zoom. Takes relative WebView coordinate
	const std::function<void(glm::vec2&)> applyZooming = [&](glm::vec2& rCoordinate)
	{
		rCoordinate -= _relativeZoomCoordinate;
		rCoordinate /= _zoom; // inverse to WebView shader
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
	// glm::vec2 coordinate;
	// if (GetOutputValue("coordinate", coordinate)) // only show when set
	// {
	// 	// TODO: convert from CEF Pixel space to WebView Pixel space
	// 	_pTab->Debug_DrawRectangle(coordinate, glm::vec2(5, 5), glm::vec3(0, 1, 0));
	// }

	// Testing visualization
	glm::vec2 testCoordinate(0.3f, 0.5f);
	applyZooming(testCoordinate);
	_pTab->Debug_DrawRectangle(testCoordinate, glm::vec2(5, 5), glm::vec3(0, 0, 1));
#endif // CLIENT_DEBUG
}

void FutureCoordinateAction::Activate()
{
	// TODO: could go wrong, as taken from weak pointer
	_spTrans = _pTab->GetCustomTransformationInterface().lock();
	_spTrans->RegisterCustomTransformation(TRANS_NAME, [](double& x, double& y) {}); // tell transformation to not transform anything, done in update
}

void FutureCoordinateAction::Deactivate()
{
	// Unregister transformation
	_spTrans->UnregisterCustomTransformation(TRANS_NAME);

	// Reset web view (necessary because of dimming)
	WebViewParameters webViewParameters;
	_pTab->SetWebViewParameters(webViewParameters);
}

void FutureCoordinateAction::Abort()
{
	_spTrans->UnregisterCustomTransformation(TRANS_NAME);
}
