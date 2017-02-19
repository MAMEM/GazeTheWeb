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
#include <algorithm>

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
	_url = URL;
	LabStreamMailer::instance().Send("Loading URL: " + _url);
}

void Tab::ReceiveFaviconBytes(std::unique_ptr< std::vector<unsigned char> > upData, int width, int height)
{
	// Go over colors and get most colorful as accent (should be always RGBA)
	int size; // width * height * 4
	if (upData != NULL && ((size = (int)upData->size()) >= 4) && width > 0 && height > 0)
	{
		// Load icon into eyeGUI
		eyegui::fetchImage(_pPanelLayout, GetFaviconIdentifier(), width, height, eyegui::ColorFormat::RGBA, upData->data(), true);
		_faviconLoaded = true;

		// Prepare loop
		int steps = (width * height) / TAB_ACCENT_COLOR_SAMPLING_POINTS;
		steps = glm::max(1, steps);
		int maxIndex = 0;
		int maxSaturation = 0;
		for (int i = 0; i < size; i += steps * 4)
		{
			// Extract colors
			float r = upData->at(i);
			float g = upData->at(i + 1);
			float b = upData->at(i + 2);
			float a = upData->at(i + 3);

			// Calculate saturation like in HSV color space
			float max = glm::max(r, glm::max(g, b));
			float saturation = 0;
			if (max != 0)
			{
				float delta = max - glm::min(r, glm::min(g, b));
				saturation = delta / max;
				saturation *= a;
			}

			// Is it maximum?
			if (maxSaturation < saturation)
			{
				maxSaturation = saturation;
				maxIndex = i;
			}
		}

		// Extract accent color
		_targetColorAccent = glm::vec4(
			((float)upData->at(maxIndex) / 255.f),
			((float)upData->at(maxIndex + 1) / 255.f),
			((float)upData->at(maxIndex + 2) / 255.f),
			1.f);

		// Check, whether new target color is too much white
		float sum = _targetColorAccent.r + _targetColorAccent.g + _targetColorAccent.b; // maximal 3
		float whiteBorder = 2.5f;
		if (sum >= whiteBorder)
		{
			// Too bright, darken it
			float multiplier = (1.f - (sum - whiteBorder));
			_targetColorAccent.r *= multiplier;
			_targetColorAccent.g *= multiplier;
			_targetColorAccent.b *= multiplier;
		}
		else if (sum <= 0.3f)
		{
			// Too dark, use default instead
			_targetColorAccent = TAB_DEFAULT_COLOR_ACCENT;
		}

        // Start color accent interpolation
        _colorInterpolation = 0;

	}
    // else: do nothing
}

void Tab::ResetFaviconBytes()
{
    _faviconLoaded = false;
}

void Tab::AddDOMNode(std::shared_ptr<DOMNode> spNode)
{
	//LogDebug("Tab: Adding new DOM node with id=", spNode->GetNodeID(), " & type=", spNode->GetType());

	// Decide what to do with node
	switch (spNode->GetType())
	{
	case DOMNodeType::TextInput:
	{
		// Add node to ID->node map
		_TextInputMap.emplace(spNode->GetNodeID(), spNode);

		// Create DOMTrigger
		std::unique_ptr<DOMTrigger> upDOMTrigger = std::unique_ptr<DOMTrigger>(new DOMTrigger(this, spNode));

		// Activate trigger
		if (!_pipelineActive)
		{
			upDOMTrigger->Activate();
		}

		// Push it to vector
		_DOMTriggers.push_back(std::move(upDOMTrigger));

		break;
	}

	case DOMNodeType::TextLink:
	{
		// Just add it to vector
		_DOMTextLinks.push_back(spNode);

		// Add node to ID->node map
		_TextLinkMap.emplace(spNode->GetNodeID(), spNode);

		break;
	}

	case DOMNodeType::SelectField:
	{
		// Just add it to vector
		_DOMSelectFields.push_back(spNode);

		// Add node to ID->node map
		_SelectFieldMap.emplace(spNode->GetNodeID(), spNode);

		break;
	}

	}
}

void Tab::ClearDOMNodes()
{
	// Deactivate all DOMTriggers
	for (auto& upDOMTrigger : _DOMTriggers)
	{
		upDOMTrigger->Deactivate();
	}

	// Clear vector with triggers
	_DOMTriggers.clear();

	// Clear vector with text links
	_DOMTextLinks.clear();

	// Clear ID->node maps
	_TextLinkMap.clear();
	_TextInputMap.clear();

	// Clear fixed elements
	_fixedElements.clear();

	// Clear overflow elements
	_overflowElements.clear();
}

void Tab::RemoveDOMNode(DOMNodeType type, int nodeID)
{

	std::map<int, std::shared_ptr<DOMNode> >* pMap = nullptr;
	std::vector<std::shared_ptr<DOMNode> >* pList = nullptr;

	switch (type)
	{
	case(DOMNodeType::TextInput) : { pMap = &_TextInputMap; break; }
	case(DOMNodeType::TextLink) : { pMap = &_TextLinkMap; pList = &_DOMTextLinks; break; }
	}

	// Remove node's ID from map
	if (pMap != nullptr && pMap->find(nodeID) != pMap->end())
	{
		pMap->erase(nodeID);
	}

	// TODO: List for all types of nodes?
	// TODO (maybe Raphael): Remove one specific DOMTrigger belonging to input field with id= nodeID
	if (pList != nullptr)
	{
		// Remove node from list of all nodes, if condition holds
		std::remove_if(
			pList->begin(),
			pList->end(),
			[nodeID](const std::shared_ptr<DOMNode>& node) {
			return node->GetNodeID() == nodeID;
		}
		);
	}
}

std::weak_ptr<DOMNode> Tab::GetDOMNode(DOMNodeType type, int nodeID)
{
	switch (type)
	{
		case DOMNodeType::TextInput: { 
			return (_TextInputMap.find(nodeID) != _TextInputMap.end()) ? _TextInputMap.at(nodeID) : std::weak_ptr<DOMNode>(); 
		}
		case DOMNodeType::TextLink: {
			return (_TextLinkMap.find(nodeID) != _TextLinkMap.end()) ? _TextLinkMap.at(nodeID) : std::weak_ptr<DOMNode>();
		}
		case DOMNodeType::SelectField: {
			return (_TextLinkMap.find(nodeID) != _TextLinkMap.end()) ? _TextLinkMap.at(nodeID) : std::weak_ptr<DOMNode>();
		}
		default: {
			LogDebug("Tab: Unknown DOMNodeType in Tab::GetDOMNode!");
		};
	}
	return std::weak_ptr<DOMNode>();
}

void Tab::SetScrollingOffset(double x, double y)
{
	// Update page size information when nearly reached end of page size (in case of endless scrollable sites like e.g. Fb.com)
	// (but don't do it if scrolling offset hasn't changed)
	/*
	auto webViewPositionAndSize = CalculateWebViewPositionAndSize();
	if ((x != _scrollingOffsetX && abs(_pageWidth - webViewPositionAndSize.width - x) < 50)
	|| (y != _scrollingOffsetY && abs(_pageHeight - webViewPositionAndSize.height - y) < 50))
	{
	_pCefMediator->GetPageResolution(this);
	}
	*/

	_scrollingOffsetX = x;
	_scrollingOffsetY = y;
}

void Tab::SetPageResolution(double width, double height)
{
	if (width != _pageWidth || height != _pageHeight)
	{
		LogInfo("Tab: Page size (w: ", _pageWidth, ", h: ", _pageHeight, ") changed to (w: ", width, ", h: ", height, ").");
	}
	_pageWidth = width;
	_pageHeight = height;
}

void Tab::AddFixedElementsCoordinates(int id, std::vector<Rect> elements)
{
	// Assign list of fixed element coordinates to given position
	if ((int)_fixedElements.size() <= id)
	{
		_fixedElements.resize(id + 1);
	}
	_fixedElements[id] = elements;

	 //DEBUG
	//LogDebug("------------------ TAB ------------------> BEGIN");
	//LogDebug("Added fixed element with id=", id);
	//LogDebug("#fixedElements: ", _fixedElements.size());
	//for (int i = 0; i < _fixedElements.size(); i++)
	//{
	//	if (_fixedElements[i].empty())
	//	{
	//		LogDebug(i, ": {}");
	//	}
	//	else if (_fixedElements[i][0].isZero())
	//	{
	//		LogDebug(i, ": 0");
	//	}
	//	else
	//	{
	//		for (const auto& rect : _fixedElements[i])
	//		{
	//			LogDebug(i, ": ", rect.toString());
	//		}
	//	}
	//}
	//LogDebug("------------------ TAB ------------------< END");
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

void Tab::SetLoadingStatus(int64 frameID, bool isMain, bool isLoading)
{
	// isMain=true && isLoading=false may indicate, that site has completely finished loading
	if (isMain)
	{
        if(isLoading)
        {
            // Main frame is loading
            eyegui::setImageOfPicture(_pPanelLayout, "icon", "icons/TabLoading_0.png");
            _timeUntilNextLoadingIconFrame = TAB_LOADING_ICON_FRAME_DURATION;
            _iconState = IconState::LOADING;
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

			// Add page to history after loading
			HistoryManager::Page page;
			page.URL = _url;
			page.title = _title;
			_pWeb->PushAddPageToHistoryJob(this, page);
        }
	}
}

void Tab::AddOverflowElement(std::shared_ptr<OverflowElement> overflowElem)
{
	_overflowElements.push_back(overflowElem);
}

std::shared_ptr<OverflowElement> Tab::GetOverflowElement(int id)
{
	if (id < (int)_overflowElements.size() && id >= 0)
	{
		return _overflowElements[id];
	}
	else
	{
		LogDebug("Tab: Error, could not find OverflowElement with id=", id);
		return nullptr;
	}

}

void Tab::RemoveOverflowElement(int id)
{
	if (id < (int)_overflowElements.size() && id >= 0)
	{
		_overflowElements[id] = NULL;
	}
	else
	{
		LogDebug("Tab: Error, could not find OverflowElement with id=", id," while trying to remove it.");
	}
}

void Tab::RequestJSDialog(JavaScriptDialogType type, std::string message)
{
	this->PushBackPipeline(std::unique_ptr<Pipeline>(new JSDialogPipeline(this, type, message)));
}