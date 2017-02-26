//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/State/Web/Tab/Tab.h"
#include "src/Master.h"
#include "src/Setup.h"
#include "src/Utils/QuadRenderItem.h"
#include "src/Utils/Helper.h"
#include "submodules/glm/glm/gtc/matrix_transform.hpp" // TODO: move to debug rendering class
#include <algorithm>
#include "src/Utils/Logger.h"

// Shaders (For debugging rectangles)
const std::string vertexShaderSource =
"#version 330 core\n"
"in vec3 posAttr;\n"
"in vec2 uvAttr;\n"
"out vec2 uv;\n"
"uniform mat4 matrix;\n"
"void main() {\n"
"   uv = uvAttr;\n"
"   gl_Position = matrix * vec4(posAttr, 1);\n"
"}\n";

const std::string fragmentShaderSource =
"#version 330 core\n"
"in vec2 uv;\n"
"out vec4 fragColor;\n"
"uniform vec3 color;\n"
"void main() {\n"
"   fragColor = vec4(color,1);\n"
"}\n";

Tab::Tab(Master* pMaster, Mediator* pCefMediator, WebTabInterface* pWeb, std::string url)
{
	// Fill members
	_pMaster = pMaster;
	_pCefMediator = pCefMediator;
	_pWeb = pWeb;
	_url = url;

	// Create layouts for Tab (overlay at first, because behind other layouts)
	_pOverlayLayout = _pMaster->AddLayout("layouts/Overlay.xeyegui", EYEGUI_TAB_LAYER, false);
	_pScrollingOverlayLayout = _pMaster->AddLayout("layouts/Overlay.xeyegui", EYEGUI_TAB_LAYER, false);
	_pPanelLayout = _pMaster->AddLayout("layouts/Tab.xeyegui", EYEGUI_TAB_LAYER, false);
	_pPipelineAbortLayout = _pMaster->AddLayout("layouts/TabPipelineAbort.xeyegui", EYEGUI_TAB_LAYER, false);
    _pDebugLayout = _pMaster->AddLayout("layouts/TabDebug.xeyegui", EYEGUI_TAB_LAYER, false);

	// Create scroll up and down floating frames in special overlay layout which always on top of standard overlay
	_scrollUpProgressFrameIndex = eyegui::addFloatingFrameWithBrick(
		_pScrollingOverlayLayout,
		"bricks/TabScrollUpProgress.beyegui",
		0.5f - (TAB_SCROLLING_SENSOR_WIDTH / 2.f),
		TAB_SCROLLING_SENSOR_PADDING,
		TAB_SCROLLING_SENSOR_WIDTH,
		TAB_SCROLLING_SENSOR_HEIGHT,
		false,
		false);
	_scrollDownProgressFrameIndex = eyegui::addFloatingFrameWithBrick(
		_pScrollingOverlayLayout,
		"bricks/TabScrollDownProgress.beyegui",
		0.5f - (TAB_SCROLLING_SENSOR_WIDTH / 2.f),
		1.f - TAB_SCROLLING_SENSOR_PADDING - TAB_SCROLLING_SENSOR_HEIGHT,
		TAB_SCROLLING_SENSOR_WIDTH,
		TAB_SCROLLING_SENSOR_HEIGHT,
		false,
		false);
	_scrollUpSensorFrameIndex = eyegui::addFloatingFrameWithBrick(
		_pScrollingOverlayLayout,
		"bricks/TabScrollUpSensor.beyegui",
		0.5f - (TAB_SCROLLING_SENSOR_WIDTH / 2.f),
		TAB_SCROLLING_SENSOR_PADDING,
		TAB_SCROLLING_SENSOR_WIDTH,
		TAB_SCROLLING_SENSOR_HEIGHT,
		false,
		false);
	_scrollDownSensorFrameIndex = eyegui::addFloatingFrameWithBrick(
		_pScrollingOverlayLayout,
		"bricks/TabScrollDownSensor.beyegui",
		0.5f - (TAB_SCROLLING_SENSOR_WIDTH / 2.f),
		1.f - TAB_SCROLLING_SENSOR_PADDING - TAB_SCROLLING_SENSOR_HEIGHT,
		TAB_SCROLLING_SENSOR_WIDTH,
		TAB_SCROLLING_SENSOR_HEIGHT,
		false,
		false);

	// Register listener
	_spTabButtonListener = std::shared_ptr<TabButtonListener>(new TabButtonListener(this));
	_spTabSensorListener = std::shared_ptr<TabSensorListener>(new TabSensorListener(this));
	eyegui::registerButtonListener(_pPanelLayout, "click_mode", _spTabButtonListener);
	eyegui::registerButtonListener(_pPanelLayout, "auto_scrolling", _spTabButtonListener);
    eyegui::registerButtonListener(_pPanelLayout, "scroll_to_top", _spTabButtonListener);
    // eyegui::registerButtonListener(_pPanelLayout, "pivot", _spTabButtonListener);
    // eyegui::registerButtonListener(_pPanelLayout, "gaze_mouse", _spTabButtonListener);
    eyegui::registerButtonListener(_pPanelLayout, "selection", _spTabButtonListener);
	eyegui::registerButtonListener(_pPanelLayout, "zoom", _spTabButtonListener);
	// eyegui::registerButtonListener(_pPanelLayout, "test_button", _spTabButtonListener); // TODO: only for testing new features
	eyegui::registerButtonListener(_pPipelineAbortLayout, "abort", _spTabButtonListener);
	eyegui::registerSensorListener(_pScrollingOverlayLayout, "scroll_up_sensor", _spTabSensorListener);
	eyegui::registerSensorListener(_pScrollingOverlayLayout, "scroll_down_sensor", _spTabSensorListener);

	// Create listener for overlay
	_spTabOverlayButtonListener = std::shared_ptr<TabOverlayButtonListener>(new TabOverlayButtonListener(this));
	_spTabOverlayKeyboardListener = std::shared_ptr<TabOverlayKeyboardListener>(new TabOverlayKeyboardListener(this));
	_spTabOverlayWordSuggestListener = std::shared_ptr<TabOverlayWordSuggestListener>(new TabOverlayWordSuggestListener(this));

	// Create WebView owned by Tab
	auto webViewInGUI = eyegui::getAbsolutePositionAndSizeOfElement(_pPanelLayout, "web_view");
    _upWebView = std::unique_ptr<WebView>(new WebView(webViewInGUI.x, webViewInGUI.y, webViewInGUI.width, webViewInGUI.height));

	// Register itself and painted texture in mediator to receive DOMNodes
	_pCefMediator->RegisterTab(this);

	// Prepare debug rendering
	_upDebugRenderItem = std::unique_ptr<RenderItem>(
		new QuadRenderItem(
			vertexShaderSource,
			fragmentShaderSource,
			Quad::Type::LINES_WITH_DIAGONAL));
}

Tab::~Tab()
{
	// Delete triggers before removing layout
	_DOMTriggers.clear();

	// Abort pipeline
	AbortAndClearPipelines();

	_pCefMediator->UnregisterTab(this);
	_pMaster->RemoveLayout(_pPanelLayout);
	_pMaster->RemoveLayout(_pPipelineAbortLayout);
	_pMaster->RemoveLayout(_pOverlayLayout);
	_pMaster->RemoveLayout(_pScrollingOverlayLayout);
    _pMaster->RemoveLayout(_pDebugLayout);
}

void Tab::Update(float tpf, Input& rInput)
{
	// #######################
	// ### UPDATE WEB VIEW ###
	// #######################

	// Update WebView (get values from eyeGUI layout directly)
	const auto webViewInGUI = eyegui::getAbsolutePositionAndSizeOfElement(_pPanelLayout, "web_view");
	_upWebView->Update(
		webViewInGUI.x,
		webViewInGUI.y,
		webViewInGUI.width,
		webViewInGUI.height);

	// ######################
	// ### UPDATE OVERLAY ###
	// ######################

	// Update click visualization
	for (int i = 0; i < (int)_clickVisualizations.size(); i++)
	{
		// Decrement visibility
		_clickVisualizations.at(i).fading -= tpf;

		// If fading done, remove from layout
		if (_clickVisualizations.at(i).fading <= 0)
		{
			eyegui::removeFloatingFrame(_pOverlayLayout, _clickVisualizations.at(i).frameIndex, false);
		}
		else
		{
			// Fade it by making it smaller
			float relativeSize = (_clickVisualizations.at(i).fading / CLICK_VISUALIZATION_DURATION) * CLICK_VISUALIZATION_RELATIVE_SIZE;

			// Position of floating frame
			float relativePositionX = (_upWebView->GetX() + _clickVisualizations.at(i).x) / (float)_pMaster->GetWindowWidth();
			float relativePositionY = (_upWebView->GetY() + _clickVisualizations.at(i).y) / (float)_pMaster->GetWindowHeight();
			relativePositionX -= relativeSize / 2.f;
			relativePositionY -= relativeSize / 2.f;

			// Tell eyeGUI about position and size of floating frame
			eyegui::setPositionOfFloatingFrame(_pOverlayLayout, _clickVisualizations.at(i).frameIndex, relativePositionX, relativePositionY);
			eyegui::setSizeOfFloatingFrame(_pOverlayLayout, _clickVisualizations.at(i).frameIndex, relativeSize, relativeSize);
		}
	}

	// Erase visualizations which are no more in layout
	_clickVisualizations.erase(
		std::remove_if(
			_clickVisualizations.begin(),
			_clickVisualizations.end(),
            [](const ClickVisualization& i) { return i.fading <= 0; }),
		_clickVisualizations.end());

	// ########################
	// ### TAB INPUT STRUCT ###
	// ########################

	// Create tab input structure (like standard input but in addition with input coordinates in web view space)
	int webViewPixelGazeX = rInput.gazeX - _upWebView->GetX();
	int webViewPixelGazeY = rInput.gazeY - _upWebView->GetY();
	float webViewGazeRelativeX = ((float)webViewPixelGazeX) / (float)(_upWebView->GetWidth());
	float webViewGazeRelativeY = ((float)webViewPixelGazeY) / (float)(_upWebView->GetHeight());
	TabInput tabInput(
		rInput.gazeX,
		rInput.gazeY,
		rInput.gazeUsed,
		rInput.instantInteraction,
		webViewPixelGazeX,
		webViewPixelGazeY,
		webViewGazeRelativeX,
		webViewGazeRelativeY);
	double CEFPixelGazeX = tabInput.webViewPixelGazeX;
	double CEFPixelGazeY = tabInput.webViewPixelGazeY;

	// Update highlight rectangle of webview
	// TODO: alternative: give webview shared pointer to DOM nodes
	std::vector<Rect> rects;
	for (const auto& rDOMNode : _DOMTextLinks)
	{
		// TODO(Daniel): Getting rid of visibility attribute
		if(true) // if (rDOMNode->GetVisibility()) // only proceed if currently visible
		{
			for (const auto& rRect : rDOMNode->GetRects())
			{
				rects.push_back(rRect);
			}
		}
	}
	_upWebView->SetHighlightRects(rects);

	// ###########################
	// ### UPDATE COLOR OF GUI ###
	// ###########################

	UpdateAccentColor(tpf);

	// ###################
	// ### UPDATE ICON ###
	// ###################

    if (_iconState == IconState::ICON_NOT_FOUND && _faviconLoaded)
	{
        // Favicon has been loaded now
        eyegui::setImageOfPicture(_pPanelLayout, "icon", GetFaviconIdentifier());
        _iconState = IconState::FAVICON;
	}
    else if (_iconState == IconState::LOADING)
	{
		// Update frame of loading icon
		_timeUntilNextLoadingIconFrame -= tpf;
		while (_timeUntilNextLoadingIconFrame < 0)
		{
			_timeUntilNextLoadingIconFrame += TAB_LOADING_ICON_FRAME_DURATION;
			_loadingIconFrame = (_loadingIconFrame + 1) % 3;
		}
		eyegui::setImageOfPicture(_pPanelLayout, "icon", "icons/TabLoading_" + std::to_string(_loadingIconFrame) + ".png");
	}

	// ###########################
    // ### UPDATE DEBUG LAYOUT ###
	// ###########################

    eyegui::setContentOfTextBlock(
        _pDebugLayout,
        "web_view_coordinate",
        "Fixed:\n"
        + std::to_string(webViewPixelGazeX) + ", " + std::to_string(webViewPixelGazeY) + "\n"
        + "Scrolled:\n"
        + std::to_string((int)(webViewPixelGazeX + _scrollingOffsetX)) + ", " + std::to_string((int)(webViewPixelGazeY + _scrollingOffsetY)));

	// #######################################
    // ### UPDATE PIPELINE OR STANDARD GUI ###
	// #######################################

	// Decide what to update
	if (_pipelineActive)
	{
		// PIPELINE GETS UPDATED. STANDARD GUI NOT VISIBLE

		// Disable input for panel layout and enable it for pipeline abort layout
		eyegui::setInputUsageOfLayout(_pPanelLayout, false);
		eyegui::setInputUsageOfLayout(_pPipelineAbortLayout, true);

		// Update current pipeline (if there is one)
		if (_pipelines.front()->Update(tpf, tabInput))
		{
			// Remove front element from pipelines
			_pipelines.front()->Deactivate();
			_pipelines.pop_front();

			// Activate new pipeline if there is some
			if (!(_pipelines.empty()))
			{
				_pipelines.front()->Activate();
			}
			else
			{
				// Remember that there is no active pipeline and deactivate GUI to abort it
				SetPipelineActivity(false);
			}
		}
	}
	else
	{
        // STANDARD GUI IS VISIBLE

        // Gaze mouse
        if(_gazeMouse && !(_pMaster->IsPaused()))
        {
            EmulateMouseCursor(tabInput.webViewPixelGazeX, tabInput.webViewPixelGazeY);
        }

		// Ask for page resolution
		_timeUntilGetPageResolution -= tpf;
		if (_timeUntilGetPageResolution <= 0)
		{
			_timeUntilGetPageResolution = TAB_GET_PAGE_RES_INTERVAL;
			_pCefMediator->GetPageResolution(this);
		}

		// Enable input for panel layout and disable it for pipeline abort layout
		eyegui::setInputUsageOfLayout(_pPanelLayout, true);
		eyegui::setInputUsageOfLayout(_pPipelineAbortLayout, false);

		// Manual scrolling
		bool showScrollUp = false;
		bool showScrollDown = false;
		if (!_autoScrolling)
		{
			// Scroll up
			if (_scrollingOffsetY > 0)
			{
				showScrollUp = true;
			}

			// Scroll down
			if ((_pageHeight - 1) > (_scrollingOffsetY + _upWebView->GetResolutionY()))
			{
				showScrollDown = true;
			}
		}

		// Set progress of scrolling
		float scrollableHeight = (_pageHeight - 1) - _upWebView->GetResolutionY();
		float progressUp = 1.f;
		float progressDown = 1.f;
		if (scrollableHeight > 0)
		{
            progressUp = _scrollingOffsetY / scrollableHeight;
            progressDown = 1.f - progressUp;
		}
		eyegui::setProgress(_pScrollingOverlayLayout, "scroll_up_progress", progressUp);
		eyegui::setProgress(_pScrollingOverlayLayout, "scroll_down_progress", progressDown);

		// Set visibility of scroll elements
		eyegui::setVisibilityOFloatingFrame(_pScrollingOverlayLayout, _scrollUpProgressFrameIndex, showScrollUp, false, true);
		eyegui::setVisibilityOFloatingFrame(_pScrollingOverlayLayout, _scrollDownProgressFrameIndex, showScrollDown, false, true);
		eyegui::setVisibilityOFloatingFrame(_pScrollingOverlayLayout, _scrollUpSensorFrameIndex, showScrollUp, false, true);
		eyegui::setVisibilityOFloatingFrame(_pScrollingOverlayLayout, _scrollDownSensorFrameIndex, showScrollDown, false, true);

		// Set activity of scroll to top button
		eyegui::setElementActivity(_pPanelLayout, "scroll_to_top", showScrollUp, true);

		// Check, that gaze is not upon a fixed element
		ConvertToCEFPixel(CEFPixelGazeX, CEFPixelGazeY);
		bool gazeUponFixed = false;
		for (const auto& rElements : _fixedElements)
		{
			for (const auto& rElement : rElements)
			{
				// Simple box test
				if(rElement.IsInside(CEFPixelGazeX, CEFPixelGazeY))
				{
					gazeUponFixed = true;
					break;
				}
			}
			if (gazeUponFixed) { break; }
		}

		// Automatic scrolling. Check whether gaze is available inside webview
		if (_autoScrolling && !tabInput.gazeUsed && tabInput.insideWebView && !gazeUponFixed)
		{
			// Only vertical scrolling supported, yet (TODO: parameters)
			float yOffset = -tabInput.webViewGazeRelativeY + 0.5f;
			yOffset *= 2; // [-1..1]
			bool negative = yOffset < 0;
			yOffset = yOffset * yOffset; // [0..1]
										 // yOffset = (glm::max(0.f, yOffset - 0.05f) / 0.95f); // In the center of view no movement
			yOffset = negative ? -yOffset : yOffset; // [-1..1]

													 // Update the auto scrolling value (TODO: not frame rate independend)
			_autoScrollingValue += tpf * (yOffset - _autoScrollingValue);
		}
		else if (_autoScrollingValue != 0)
		{
			// Fade it out since auto scrolling still ongoing
			if (_autoScrollingValue > 0)
			{
				_autoScrollingValue -= tpf;
				_autoScrollingValue = glm::max(0.f, _autoScrollingValue);
			}
			else
			{
				_autoScrollingValue += tpf;
				_autoScrollingValue = glm::min(0.f, _autoScrollingValue);
			}
		}

		// Use value of auto scrolling to scroll
        if(_autoScrollingValue != 0.0f)
        {
            _pCefMediator->EmulateMouseWheelScrolling(this, 0.0, (double)(20.f * _autoScrollingValue));
        }

		// Autoscroll inside of OverflowElement if gazed upon
		bool overflowScrolling = false;
		for (const auto& rOverflowElement : _overflowElements)
		{
			if (rOverflowElement)
			{
				for (const auto& rRect : rOverflowElement->GetRects())
				{
					int scrolledCEFPixelGazeX = CEFPixelGazeX;
					int scrolledCEFPixelGazeY = CEFPixelGazeY;

					// Do NOT add scrolling offset if element is fixed
					if (!rOverflowElement->GetFixed())
					{
						scrolledCEFPixelGazeX += _scrollingOffsetX;
						scrolledCEFPixelGazeY += _scrollingOffsetY;
					}

					// Check if current gaze is inside of overflow element, if so execute scrolling method in corresponding Javascript object
					if (rRect.IsInside(scrolledCEFPixelGazeX, scrolledCEFPixelGazeY))
					{
						ScrollOverflowElement(rOverflowElement->GetId(), CEFPixelGazeX, CEFPixelGazeY);
						break;
					}
				}
				if (overflowScrolling)
				{
					break;
				}
			}
		}
		//LogDebug(tabInput.webViewGazeX, "\t",tabInput.webViewGazeY);

		// #######################
		// ### Update triggers ###
		// #######################

		for (auto& upDOMTrigger : _DOMTriggers)
		{
			upDOMTrigger->Update(tpf, tabInput);
		}
	}
}

void Tab::Draw() const
{
	// Draw WebView
	_upWebView->Draw(
		_webViewParameters,
		_pMaster->GetWindowWidth(),
		_pMaster->GetWindowHeight(),
		_scrollingOffsetX,
		_scrollingOffsetY);

	// Decide what to draw
	if (_pipelineActive)
	{
		// Draw stuff from current pipeline (overlay etc.)
		_pipelines.front()->Draw();
	}
	else
	{
		// Draw triggers
		for (const auto& upDOMTrigger : _DOMTriggers)
		{
			upDOMTrigger->Draw();
		}

		// Draw debug overlay (do not so while pipeline is rendered)
		if (setup::DRAW_DEBUG_OVERLAY)
		{
			DrawDebuggingOverlay();
		}
	}
}

void Tab::Activate()
{
	// Show layouts
	eyegui::setVisibilityOfLayout(_pOverlayLayout, true, true, false);
	eyegui::setVisibilityOfLayout(_pScrollingOverlayLayout, true, true, false);
	eyegui::setVisibilityOfLayout(_pPanelLayout, true, true, false);
    eyegui::setVisibilityOfLayout(_pDebugLayout, setup::DRAW_DEBUG_OVERLAY, true, false);

	// Setup switches
	if (_autoScrolling) { eyegui::buttonDown(_pPanelLayout, "auto_scrolling", true); }
	if (_zoomLevel != 1.0) { eyegui::buttonDown(_pPanelLayout, "zoom", true); }

	// Screw color interpolation
	_currentColorAccent = _targetColorAccent;
	_colorInterpolation = 1.f;
}

void Tab::Deactivate()
{
	// Hide layouts
	eyegui::setVisibilityOfLayout(_pOverlayLayout, false, true, false);
	eyegui::setVisibilityOfLayout(_pScrollingOverlayLayout, false, true, false);
	eyegui::setVisibilityOfLayout(_pPanelLayout, false, true, false);
    eyegui::setVisibilityOfLayout(_pDebugLayout, false, true, false);

	// TODO: THIS SHOULD NOT BE NECESSARY SINCE _pScrollingOverlayLayout IS HIDDEN! WHY?
	eyegui::setVisibilityOFloatingFrame(_pScrollingOverlayLayout, _scrollUpProgressFrameIndex, false, false, true);
	eyegui::setVisibilityOFloatingFrame(_pScrollingOverlayLayout, _scrollDownProgressFrameIndex, false, false, true);
	eyegui::setVisibilityOFloatingFrame(_pScrollingOverlayLayout, _scrollUpSensorFrameIndex, false, false, true);
	eyegui::setVisibilityOFloatingFrame(_pScrollingOverlayLayout, _scrollDownSensorFrameIndex, false, false, true);

	// Abort pipeline, which also hides GUI for manual abortion
	AbortAndClearPipelines();
}

void Tab::OpenURL(std::string URL)
{
	// Set URL
	_url = URL;

	// Abort any pipeline execution
	AbortAndClearPipelines();

	// Tell CEF to refresh this tab (which will fetch the URL and load it)
	_pCefMediator->RefreshTab(this);

	// Reset scrolling
	_scrollingOffsetX = 0.0;
	_scrollingOffsetY = 0.0;
}

void Tab::AbortAndClearPipelines()
{
	// Clear pipeline
	_pipelines.clear(); // Destructor of pipeline aborts them
	SetPipelineActivity(false);
}

void Tab::GoForward()
{
	_pCefMediator->GoForward(this);
}

void Tab::GoBack()
{
	_pCefMediator->GoBack(this);
}

void Tab::Reload()
{
	_pCefMediator->ReloadTab(this);
	this->AbortAndClearPipelines();
}

void Tab::SetPipelineActivity(bool active)
{
	if (active)
	{
		_pipelineActive = true;
		eyegui::setVisibilityOfLayout(_pPipelineAbortLayout, true, true, true);

		// Deactivate all triggers
		for (auto& upDOMTrigger : _DOMTriggers)
		{
			upDOMTrigger->Deactivate();
		}

		// Deactivate scrolling overlay
		eyegui::setVisibilityOfLayout(_pScrollingOverlayLayout, false, false, true);
	}
	else
	{
		_pipelineActive = false;
		eyegui::setVisibilityOfLayout(_pPipelineAbortLayout, false, false, true);

		// Activate all triggers
		for (auto& upDOMTrigger : _DOMTriggers)
		{
			upDOMTrigger->Activate();
		}

		// Activate scrolling overlay
		eyegui::setVisibilityOfLayout(_pScrollingOverlayLayout, true, true, true);
	}
}

void Tab::UpdateAccentColor(float tpf)
{
	// Move accent color accent towards target accent color
	glm::vec4 colorAccent = _currentColorAccent;
	if (_colorInterpolation < 1.f)
	{
		_colorInterpolation += tpf; // interpolate
		_colorInterpolation = glm::min(_colorInterpolation, 1.f); // clamp
		colorAccent = ((1.f - _colorInterpolation) * _currentColorAccent) + (_colorInterpolation * _targetColorAccent); // calculation of current color
		if (_colorInterpolation == 1.f)
		{
			_currentColorAccent = _targetColorAccent;
		}
	}

	// Create background color
	float backgroundMultiplier = 0.5f;
	float panelBackgroundMultiplier = 0.4f;
	glm::vec4 backgroundAccentColor(
		colorAccent.r * backgroundMultiplier,
		colorAccent.g * backgroundMultiplier,
		colorAccent.b * backgroundMultiplier,
		colorAccent.a);
	glm::vec4 panelBackgroundAccentColor(
		colorAccent.r * panelBackgroundMultiplier,
		colorAccent.g * panelBackgroundMultiplier,
		colorAccent.b * panelBackgroundMultiplier,
		colorAccent.a);

	// Create transparent colors
	float alpha = 0.75f;
	float backgroundAlpha = 0.5f;
	glm::vec4 transparentColorAccent = glm::vec4(
		colorAccent.r,
		colorAccent.g,
		colorAccent.b,
		colorAccent.a * alpha);
	glm::vec4 transparentBackgroundColorAccent = glm::vec4(
		backgroundAccentColor.r,
		backgroundAccentColor.g,
		backgroundAccentColor.b,
		backgroundAccentColor.a * backgroundAlpha);

	// Set color of tab_panel style in pipeline abort overlay
	eyegui::setValueOfStyleAttribute(
		_pPipelineAbortLayout,
		"tab_panel",
		"color",
		RGBAToHexString(colorAccent));

	// Set color of tab_panel style in layout
	eyegui::setValueOfStyleAttribute(
		_pPanelLayout,
		"tab_panel",
		"color",
		RGBAToHexString(colorAccent));

	// Set color of tab_overlay style in overlay layout
	eyegui::setValueOfStyleAttribute(
		_pOverlayLayout,
		"tab_overlay",
		"color",
		RGBAToHexString(colorAccent));
	eyegui::setValueOfStyleAttribute(
		_pOverlayLayout,
		"tab_overlay",
		"background-color",
		RGBAToHexString(backgroundAccentColor));

	// Set color of tab_overlay_noback style in overlay layout
	eyegui::setValueOfStyleAttribute(
		_pOverlayLayout,
		"tab_overlay_noback",
		"color",
		RGBAToHexString(colorAccent));
	eyegui::setValueOfStyleAttribute(
		_pOverlayLayout,
		"tab_overlay_noback",
		"color",
		RGBAToHexString(colorAccent));

	// Set color of tab_overlay_panel style in overlay layout
	eyegui::setValueOfStyleAttribute(
		_pOverlayLayout,
		"tab_overlay_panel",
		"color",
		RGBAToHexString(backgroundAccentColor));
	eyegui::setValueOfStyleAttribute(
		_pOverlayLayout,
		"tab_overlay_panel",
		"background-color",
		RGBAToHexString(panelBackgroundAccentColor));

	// Set color of tab_overlay style in scrolling overlay layout
	eyegui::setValueOfStyleAttribute(
		_pScrollingOverlayLayout,
		"tab_overlay",
		"color",
		RGBAToHexString(colorAccent));

	// Set color and background of tab_overlay_scroll_progress style in scrolling overlay layout
	eyegui::setValueOfStyleAttribute(
		_pScrollingOverlayLayout,
		"tab_overlay_scroll_progress",
		"color",
		RGBAToHexString(transparentColorAccent));
	eyegui::setValueOfStyleAttribute(
		_pScrollingOverlayLayout,
		"tab_overlay_scroll_progress",
		"background-color",
		RGBAToHexString(transparentBackgroundColorAccent));
}

void Tab::DrawDebuggingOverlay() const
{
	// Reserve variables
	glm::mat4 model, matrix;

	// Projection
	glm::mat4 projection = glm::ortho(0, 1, 0, 1);

	// Define render function
	std::function<void(Rect, bool)> renderRect = [&](Rect rect, bool fixed)
	{
		// Fixed or not
		if (!fixed)
		{
			// Subtract scrolling while coordinates are in CEFPixel space
			rect.left -= _scrollingOffsetX;
			rect.right -= _scrollingOffsetX;
			rect.bottom -= _scrollingOffsetY;
			rect.top -= _scrollingOffsetY;
		}

		// Scale from CEFPixel space to WebViewPixel
		rect.left = (rect.left / (float)_upWebView->GetResolutionX()) * (float)_upWebView->GetWidth();
		rect.right = (rect.right / (float)_upWebView->GetResolutionX()) * (float)_upWebView->GetWidth();
		rect.bottom = (rect.bottom / (float)_upWebView->GetResolutionY()) * (float)_upWebView->GetHeight();
		rect.top = (rect.top / (float)_upWebView->GetResolutionY()) * (float)_upWebView->GetHeight();

		// Calculate model matrix
		model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(1.f / _pMaster->GetWindowWidth(), 1.f / _pMaster->GetWindowHeight(), 1.f));
		model = glm::translate(model, glm::vec3(_upWebView->GetX() + rect.left, _upWebView->GetHeight() - (rect.bottom), 1));
		model = glm::scale(model, glm::vec3(rect.Width(), rect.Height(), 0));

		// Combine matrics
		matrix = projection * model;

		// Fill uniform with matrix (no need for Bind() since bound in called context)
		_upDebugRenderItem->GetShader()->UpdateValue("matrix", matrix);

		// Render rectangle
		_upDebugRenderItem->Draw(GL_LINES);
	};

	// Bind debug render item
	_upDebugRenderItem->Bind();

	// ### DOMTRIGGER ###

	// Set rendering up for DOMTrigger
	_upDebugRenderItem->GetShader()->UpdateValue("color", DOM_TRIGGER_DEBUG_COLOR);

	// Go over all DOMTriggers
	for (const auto& rDOMTrigger : _DOMTriggers)
	{
		// Render rects
		for (const auto rRect : rDOMTrigger->GetDOMRects())
		{
			if (true)//rDOMTrigger->GetDOMVisibility())
			{
				if (!rDOMTrigger->GetDOMIsPasswordField())
				{
					renderRect(rRect, rDOMTrigger->GetDOMFixed());
				}
				else
				{
					_upDebugRenderItem->GetShader()->UpdateValue("color", glm::vec3(0.0f, 1.f, 1.f));
					renderRect(rRect, rDOMTrigger->GetDOMFixed());
					_upDebugRenderItem->GetShader()->UpdateValue("color", DOM_TRIGGER_DEBUG_COLOR);
				}
			}
				
		}
	}

	// ### DOMTEXTLINKS ###

	// Set rendering up for DOMTextLink
	_upDebugRenderItem->GetShader()->UpdateValue("color", DOM_TEXT_LINKS_DEBUG_COLOR);

	// Go over all DOMTextLinks
	for (const auto& rDOMTextLink : _DOMTextLinks)
	{
		// Render rects
		for (const auto rRect : rDOMTextLink->GetRects())
		{
			if(rDOMTextLink->GetVisibility())
				renderRect(rRect, rDOMTextLink->GetFixed());
			else
			{
				_upDebugRenderItem->GetShader()->UpdateValue("color", glm::vec3(1.0f, 0.f, 1.f));
				renderRect(rRect, rDOMTextLink->GetFixed());
				_upDebugRenderItem->GetShader()->UpdateValue("color", DOM_TEXT_LINKS_DEBUG_COLOR);
			}
		}
	}

	// DEBUG - links containing line break are shown in another color
	_upDebugRenderItem->GetShader()->UpdateValue("color", glm::vec3(1.f, 0.f, 1.f));
	for (const auto& rDOMTextLink : _DOMTextLinks)
	{
		if(rDOMTextLink->GetRects().size() == 2 && rDOMTextLink->GetVisibility())
			renderRect(rDOMTextLink->GetRects()[1], rDOMTextLink->GetFixed());
	}

	// ### SELECT FIELDS ###
	// Set rendering up for DOMSelectFields
	_upDebugRenderItem->GetShader()->UpdateValue("color", DOM_SELECT_FIELD_DEBUG_COLOR);
	for (const auto& rDOMSelectField : _DOMSelectFields)
	{
		// Render rects
		for (const auto rRect : rDOMSelectField->GetRects())
		{
			renderRect(rRect, rDOMSelectField->GetFixed());
		}
	}


	// ### FIXED ELEMENTS ###

	// Set rendering up for fixed element
	_upDebugRenderItem->GetShader()->UpdateValue("color", FIXED_ELEMENT_DEBUG_COLOR);

	// Go over all fixed elements vectors
	for (const auto& rFixedElements : _fixedElements)
	{
		// Go over fixed elements
		for (const auto& rFixedElement : rFixedElements)
		{
			// Render rect
			if(rFixedElement.Height() > 0 && rFixedElement.Width() > 0)
				renderRect(rFixedElement, true);
		}
	}

	// ### OVERFLOW ELEMENTS ###
	_upDebugRenderItem->GetShader()->UpdateValue("color", glm::vec3(255.f/255.f, 127.f/255.f, 35.f/255.f));

	for (const auto& rOverflowElement : _overflowElements)
	{
		if (rOverflowElement) // Note: Can be NULL if aquivalent element in JS got deleted. (see Tab::RemoveOverflowElement)
		{
			for (const auto& rect : rOverflowElement->GetRects())
				renderRect(rect, rOverflowElement->GetFixed());
		}

	}
}

void Tab::PushBackClickVisualization(double x, double y)
{
	// Structure for click visulization
	ClickVisualization clickVisualization;
	clickVisualization.x = x;
	clickVisualization.y = y;
	clickVisualization.fading = CLICK_VISUALIZATION_DURATION;

	// Position of floating frame
	float relativePositionX = (_upWebView->GetX() + x) / (float)_pMaster->GetWindowWidth();
	float relativePositionY = (_upWebView->GetY() + y) / (float)_pMaster->GetWindowHeight();
	relativePositionX -= CLICK_VISUALIZATION_RELATIVE_SIZE / 2.f;
	relativePositionY -= CLICK_VISUALIZATION_RELATIVE_SIZE / 2.f;

	// Add floating frame to overlay
	clickVisualization.frameIndex = eyegui::addFloatingFrameWithBrick(
		_pOverlayLayout,
		"bricks/TabClickVisualization.beyegui",
		relativePositionX,
		relativePositionY,
		CLICK_VISUALIZATION_RELATIVE_SIZE,
		CLICK_VISUALIZATION_RELATIVE_SIZE,
		true,
		false);

	// Add to vector which is updated per frame
	_clickVisualizations.push_back(clickVisualization);
}

std::string Tab::GetFaviconIdentifier() const
{
	return "tab_info_" + std::to_string(_pWeb->GetIdOfTab(this));
}