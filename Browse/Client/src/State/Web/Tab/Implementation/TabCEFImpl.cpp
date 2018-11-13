//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/State/Web/Tab/Tab.h"
#include "src/Utils/Logger.h"
#include "src/Setup.h"
#include "src/State/Web/Managers/HistoryManager.h"
#include "src/State/Web/Tab/Pipelines/JSDialogPipeline.h"
#include "src/Singletons/LabStreamMailer.h"
#include "src/Singletons/FirebaseMailer.h"
#include "src/CEF/Mediator.h"
#include <algorithm>

#define SendRenderMessage [this](CefRefPtr<CefProcessMessage> msg) { return _pCefMediator->SendProcessMessageToRenderer(msg, this); }

void Tab::GetWebRenderResolution(int& rWidth, int& rHeight) const
{
	// One cannot ask the web view for its size, because its size is not updated before the update call
	// of tab happens in the next frame. But this method is called by CEF directly at resize callback,
	// which means before the update call. Ideas for improvement are welcomed
	auto webViewInGUI = eyegui::getAbsolutePositionAndSizeOfElement(_pPanelLayout, "web_view");
	rWidth = webViewInGUI.width * setup::WEB_VIEW_RESOLUTION_SCALE;
	rHeight = webViewInGUI.height * setup::WEB_VIEW_RESOLUTION_SCALE;
}

void Tab::SetURL(std::string URL)
{
	// Set URL
	_url = URL;

	// Reset title
	_title = "";

	// Check whether current history page entry contains same URL. If not, create new one
	if (_spHistoryPage == nullptr || _spHistoryPage->GetURL() != _url)
	{
		if (_dataTransfer)
		{
			// Add history entry for new url with current title, will be updated asap
			_spHistoryPage = _pWeb->AddPageToHistory(_url, _title);
		}
	}
}

void Tab::ReceiveFaviconBytes(std::unique_ptr< std::vector<unsigned char> > upData, int width, int height)
{
	// Go over colors and get most colorful as accent (should be always RGBA)
	int size; // width * height * 4
	if (upData != NULL && ((size = (int)upData->size()) >= 4) && width > 0 && height == width) // only accept square icons
	{
		if (size <= _current_favicon_bytes)
			return;

		LogInfo("Tab: Current favicon resolution -- ", width, " x ", height);
		_current_favicon_bytes = size;

		// Load icon into eyeGUI
		eyegui::fetchImage(_pPanelLayout, GetFaviconIdentifier(), width, height, eyegui::ColorFormat::RGBA, upData->data(), true);
		_faviconLoaded = true;

		// Prepare loop
		int steps = (width * height) / TAB_ACCENT_COLOR_SAMPLING_POINTS;
		steps = glm::max(1, steps);
		int maxIndex = 0;
		int maxSaturation = 0;
		bool foundColor = false;
		for (int i = 0; i < size; i += steps * 4)
		{
			// Discard pixels that are transparent
			if (upData->at(i + 3) < 200)
			{
				continue;
			}
			else
			{
				foundColor = true;
			}

			// Extract colors
			float r = upData->at(i);
			float g = upData->at(i + 1);
			float b = upData->at(i + 2);
			// float a = upData->at(i + 3); // not used

			// Calculate saturation like in HSV color space
			float max = glm::max(r, glm::max(g, b));
			float saturation = 0;
			if (max != 0)
			{
				float delta = max - glm::min(r, glm::min(g, b));
				saturation = delta / max;
				// saturation *= a; // Are values already premultiplied?
			}

			// Is it maximum?
			if (maxSaturation < saturation)
			{
				maxSaturation = saturation;
				maxIndex = i;
			}
		}

		if (foundColor)
		{
			// Extract accent color
			_targetColorAccent = glm::vec4(
				((float)upData->at(maxIndex) / 255.f),
				((float)upData->at(maxIndex + 1) / 255.f),
				((float)upData->at(maxIndex + 2) / 255.f),
				1.f);

			// Check, whether new target color is too much white
			float sum = _targetColorAccent.r + _targetColorAccent.g + _targetColorAccent.b; // maximal 3
			float whiteBorder = 2.0;
			if (sum >= whiteBorder)
			{
				// Too bright, darken it
				float multiplier = (1.f - ((sum - whiteBorder)/3.f));
				_targetColorAccent.r *= multiplier;
				_targetColorAccent.g *= multiplier;
				_targetColorAccent.b *= multiplier;
			}
			else if (sum <= 0.3f)
			{
				// Too dark, use default instead
				_targetColorAccent = TAB_DEFAULT_COLOR_ACCENT;
			}
		}
		else
		{
			_targetColorAccent = TAB_DEFAULT_COLOR_ACCENT;
		}

        // Start color accent interpolation
        _colorInterpolation = 0;

	}
    // else: do nothing
}

void Tab::ResetFaviconBytes()
{
	LogInfo("Tab: ResetFaviconBytes called!");
    _faviconLoaded = false;
	_current_favicon_bytes = 0;
	_loaded_favicon_urls.clear();
}

bool Tab::IsFaviconAlreadyAvailable(std::string img_url)
{
	if (std::find(_loaded_favicon_urls.begin(), _loaded_favicon_urls.end(), img_url) == _loaded_favicon_urls.end())
	{
		_loaded_favicon_urls.push_back(img_url);
		return false;
	}
	return true;
}

void Tab::AddDOMTextInput(int id)
{
	std::shared_ptr<DOMTextInput> spNode = std::make_shared<DOMTextInput>(id, this);

	// Add node to ID->node map
	_TextInputMap.emplace(id, spNode);

	// Create DOMTrigger
	std::unique_ptr<TextInputTrigger> upDOMTrigger = std::unique_ptr<TextInputTrigger>(new TextInputTrigger(this, _triggers, spNode));

	// Activate trigger
	if (!_pipelineActive)
	{
		upDOMTrigger->Activate();
	}

	// Place trigger in map
	_textInputTriggers.emplace(id, std::move(upDOMTrigger));
}

void Tab::AddDOMLink(int id)
{
	_TextLinkMap.emplace(id, std::make_shared<DOMLink>(id));
}

void Tab::AddDOMSelectField(int id)
{
	std::shared_ptr<DOMSelectField> spNode = std::make_shared<DOMSelectField>(id, this);

	// Add node to ID->node map
	_SelectFieldMap.emplace(id, spNode);

	// Create DOMTrigger
	std::unique_ptr<SelectFieldTrigger> upDOMTrigger = std::unique_ptr<SelectFieldTrigger>(new SelectFieldTrigger(this, _triggers, spNode));

	// Activate trigger
	if (!_pipelineActive)
	{
		upDOMTrigger->Activate();
	}

	// Place trigger in map
	_selectFieldTriggers.emplace(id, std::move(upDOMTrigger));
}

void Tab::AddDOMOverflowElement(int id)
{
	_OverflowElementMap.emplace(id, std::make_shared<DOMOverflowElement>(id, this));
}

void Tab::AddDOMVideo(int id)
{
	std::shared_ptr<DOMVideo> spNode = std::make_shared<DOMVideo>(id, this);

	// Add node to ID->node map
	_VideoMap.emplace(id, spNode);

	// Create DOMTrigger
	std::unique_ptr<VideoModeTrigger> upDOMTrigger = std::unique_ptr<VideoModeTrigger>(new VideoModeTrigger(this, _triggers, spNode,
		[&](int id)
	{
		this->EnterVideoMode(id);
	}));

	// Activate trigger
	if (!_pipelineActive)
	{
		upDOMTrigger->Activate();
	}

	// Place trigger in map
	_videoModeTriggers.emplace(id, std::move(upDOMTrigger));
}

void Tab::AddDOMCheckbox(int id)
{
	_CheckboxMap.emplace(id, std::make_shared<DOMCheckbox>(id, this));
}


std::weak_ptr<DOMTextInput> Tab::GetDOMTextInput(int id)
{
	return (_TextInputMap.find(id) != _TextInputMap.end()) ? _TextInputMap.at(id) : std::weak_ptr<DOMTextInput>();
}

std::weak_ptr<DOMLink> Tab::GetDOMLink(int id)
{
	return (_TextLinkMap.find(id) != _TextLinkMap.end()) ? _TextLinkMap.at(id) : std::weak_ptr<DOMLink>();
}

std::weak_ptr<DOMSelectField> Tab::GetDOMSelectField(int id)
{
	return (_SelectFieldMap.find(id) != _SelectFieldMap.end()) ? _SelectFieldMap.at(id) : std::weak_ptr<DOMSelectField>();
}

std::weak_ptr<DOMOverflowElement> Tab::GetDOMOverflowElement(int id)
{
	return (_OverflowElementMap.find(id) != _OverflowElementMap.end()) ? _OverflowElementMap.at(id) : std::weak_ptr<DOMOverflowElement>();
}

std::weak_ptr<DOMVideo> Tab::GetDOMVideo(int id)
{
	return (_VideoMap.find(id) != _VideoMap.end()) ? _VideoMap.at(id) : std::weak_ptr<DOMVideo>();
}

std::weak_ptr<DOMCheckbox> Tab::GetDOMCheckbox(int id)
{
	return (_CheckboxMap.find(id) != _CheckboxMap.end()) ? _CheckboxMap.at(id) : std::weak_ptr<DOMCheckbox>();
}


void Tab::ClearDOMNodes()
{
	// Deactivate all triggers
	for (auto pTrigger : _triggers)
	{
		pTrigger->Deactivate();
	}

	// Clear vector with triggers
	_textInputTriggers.clear();
	_selectFieldTriggers.clear();
	_videoModeTriggers.clear();

	// Clear ID->node maps
	_TextLinkMap.clear();
	_TextInputMap.clear();
	_SelectFieldMap.clear();
	_VideoMap.clear();
	_CheckboxMap.clear();

	// Clear fixed elements
	_fixedElements.clear();

	// Clear overflow elements
	_OverflowElementMap.clear();
}

void Tab::RemoveDOMTextInput(int id)
{
	if (_textInputTriggers.find(id) != _textInputTriggers.end()) { _textInputTriggers.erase(id); }
	if (_TextInputMap.find(id) != _TextInputMap.end()) { _TextInputMap.erase(id); }
}

void Tab::RemoveDOMLink(int id)
{
	if (_TextLinkMap.find(id) != _TextLinkMap.end()) { _TextLinkMap.erase(id); }
}

void Tab::RemoveDOMSelectField(int id)
{
	if (_selectFieldTriggers.find(id) != _selectFieldTriggers.end()) { _selectFieldTriggers.erase(id); }
	if (_SelectFieldMap.find(id) != _SelectFieldMap.end()) { _SelectFieldMap.erase(id); }
}

void Tab::RemoveDOMOverflowElement(int id)
{
	if (_OverflowElementMap.find(id) != _OverflowElementMap.end()) { _OverflowElementMap.erase(id); }
}

void Tab::RemoveDOMVideo(int id)
{
	// Exit video mode if currently showing the video
	if (id == _videoModeId)
	{
		ExitVideoMode();
	}

	if (_videoModeTriggers.find(id) != _videoModeTriggers.end()) { _videoModeTriggers.erase(id); }
	if (_VideoMap.find(id) != _VideoMap.end()) { _VideoMap.erase(id); }
}

void Tab::RemoveDOMCheckbox(int id)
{
	if (_CheckboxMap.find(id) != _CheckboxMap.end()) { _CheckboxMap.erase(id); }
}



void Tab::SetScrollingOffset(double x, double y)
{
	_scrollingOffsetX = x;
	_scrollingOffsetY = y;
}

void Tab::SetPageResolution(double width, double height)
{
	if (width != _pageWidth || height != _pageHeight)
	{
		LogInfo("Tab: Page size (w: ", _pageWidth, ", h: ", _pageHeight, ") changed to (w: ", width, ", h: ", height, ").");
		_pageWidth = width;
		_pageHeight = height;
	}
}

void Tab::AddFixedElementsCoordinates(int id, std::vector<Rect> elements)
{
	// Assign list of fixed element coordinates to given position
	if ((int)_fixedElements.size() <= id)
	{
		_fixedElements.resize(id + 1);
	}
	_fixedElements[id] = elements;

}

void Tab::RemoveFixedElement(int id)
{
	// DEBUG
	//LogDebug("Tab: Remove fixed element id=", id);

	if ((int)_fixedElements.size() > id)
	{
		_fixedElements[id].clear();
	}
	//else
	//{
	//	LogDebug("Tab: Fixed element with id=", id, " wasn't in list...");
	//}
}

void Tab::SetTitle(std::string title)
{
	_title = title;
	if (_spHistoryPage != nullptr)
	{
		_spHistoryPage->SetTitle(_title);
	}
}

void Tab::SetLoadingStatus(bool isLoading, bool isMainFrame)
{
	if (!isMainFrame)
		return;

	// isLoading=false may indicate, that site has completely finished loading
	if(isLoading)
    {
        // Main frame is loading
        eyegui::setImageOfPicture(_pPanelLayout, "icon", "icons/TabLoading_0.png");
        _timeUntilNextLoadingIconFrame = TAB_LOADING_ICON_FRAME_DURATION;
        _iconState = IconState::LOADING;

		// Abort any pipeline execution when loading of main frame starts
		AbortAndClearPipelines();

		// Tell lab stream layer
		LabStreamMailer::instance().Send("Start Loading New URL");
    }
    else
    {
        // Main frame is done with loading
        if (_faviconLoaded)
        {
            eyegui::setImageOfPicture(_pPanelLayout, "icon", GetFaviconIdentifier());
            _iconState = IconState::FAVICON;
        }
        else
        {
            eyegui::setImageOfPicture(_pPanelLayout, "icon", "icons/TabIconNotFound.png");
            _iconState = IconState::ICON_NOT_FOUND;
        }

		// Check whether it was the dashboard and whether to update the award in Web
		if (_url.find(setup::DASHBOARD_URL) != std::string::npos)
		{
			// Update award TODO: will update on every subpage of the dashboard. maybe improve on that
			_pWeb->PushUpdateAwardJob(this, FirebaseMailer::Instance().GetUserAward());
		}

		// Tell lab stream layer
		LabStreamMailer::instance().Send("Finished Loading URL: " + _url);
    }
}

void Tab::RequestJSDialog(JavaScriptDialogType type, std::string message)
{
	this->PushBackPipeline(std::unique_ptr<Pipeline>(new JSDialogPipeline(this, type, message)));
}

void Tab::SetMetaKeywords(std::string content)
{
	_metaKeywords = content;
}