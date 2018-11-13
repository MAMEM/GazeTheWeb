//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/State/Web/Tab/Tab.h"
#include "src/CEF/Mediator.h"
#include "src/Utils/Texture.h"
#include "src/Singletons/LabStreamMailer.h"
#include "src/Master/Master.h"
#include "src/State/Web/Tab/SocialRecord.h"
#include <locale>
#include <memory>
#include <codecvt>
#include <string>
#include <iostream>

void Tab::PushBackPipeline(std::unique_ptr<Pipeline> upPipeline)
{
	_pipelines.push_back(std::move(upPipeline));
	if (!_pipelineActive)
	{
		// Activate first pipeline
		_pipelines.front()->Activate();

		// Remember that there is active pipeline and activate GUI to abort it
		SetPipelineActivity(true);
	}
}

void Tab::EmulateLeftMouseButtonClick(double x, double y, bool visualize, bool isWebViewPixelCoordinate, bool userTriggered)
{
	// Add some visualization for the user at screen position
	if (visualize)
	{
		// Maybe convert (back) into WebViewPixel system
		double WebViewPixelX = x;
		double WebViewPixelY = y;
		if (!isWebViewPixelCoordinate)
		{
			ConvertToWebViewPixel(WebViewPixelX, WebViewPixelY);
		}
		PushBackClickVisualization(WebViewPixelX, WebViewPixelY);
	}

	// To CEFPixel coordinates
	if (isWebViewPixelCoordinate)
	{
		ConvertToCEFPixel(x, y);
	}

	LabStreamMailer::instance().Send("Click performed");

	// Store information whether this click was triggered by the user
	_userTriggeredClick = userTriggered;

	// Tell mediator about the click
	_pCefMediator->EmulateLeftMouseButtonClick(this, x, y);
}

void Tab::EmulateMouseCursor(double x, double y, bool leftButtonPressed, bool isWebViewPixelCoordinate, double xOffset, double yOffset)
{
	// To CEFPixel coordinates
	if (isWebViewPixelCoordinate)
	{
		ConvertToCEFPixel(x, y);
	}

	// Tell mediator about the cursor
	_pCefMediator->EmulateMouseCursor(this, x + xOffset, y + yOffset, leftButtonPressed);
}

void Tab::EmulateMouseWheelScrolling(double deltaX, double deltaY)
{
	_pCefMediator->EmulateMouseWheelScrolling(this, deltaX, deltaY);
}

void Tab::EmulateLeftMouseButtonDown(double x, double y, bool isWebViewPixelCoordinate, double xOffset, double yOffset)
{
	// To CEFPixel coordinates
	if (isWebViewPixelCoordinate)
	{
		ConvertToCEFPixel(x, y);
	}

	// Tell mediator about mouse button down
	_pCefMediator->EmulateLeftMouseButtonDown(this, x + xOffset, y + yOffset);
}

void Tab::EmulateLeftMouseButtonUp(double x, double y, bool isWebViewPixelCoordinate, double xOffset, double yOffset)
{
	// To CEFPixel coordinates
	if (isWebViewPixelCoordinate)
	{
		ConvertToCEFPixel(x, y);
	}

	// Tell mediator about mouse button up
	_pCefMediator->EmulateLeftMouseButtonUp(this, x + xOffset, y + yOffset);
}

void Tab::PutTextSelectionToClipboardAsync()
{
	_pCefMediator->PutTextSelectionToClipboardAsync(this);
}

std::string Tab::GetClipboardText() const
{
	return _pCefMediator->GetClipboardText();
}

std::weak_ptr<const DOMNode> Tab::GetNearestLink(glm::vec2 pagePixelCoordinate, float& rDistance) const
{
    if(_TextLinkMap.empty())
    {
        // No link available
        rDistance = -1;
        return std::weak_ptr<DOMNode>();
    }
    else
    {
        // Get link with minimal distance
        float minDistance = std::numeric_limits<float>::max();
        std::weak_ptr<const DOMNode> wpResult;

        // Go over links
        for(const auto& idLinkPair : _TextLinkMap)
        {
            // Go over rectangles of that link
            for(const auto& rRect : idLinkPair.second->GetRects())
            {
                // Distance
                float dx = glm::max(glm::abs(pagePixelCoordinate.x - rRect.Center().x) - (rRect.Width() / 2.f), 0.f);
                float dy = glm::max(glm::abs(pagePixelCoordinate.y - rRect.Center().y) - (rRect.Height() / 2.f), 0.f);
                float distance = glm::sqrt((dx * dx) + (dy * dy));

                // Check whether distance is smaller
                if(distance < minDistance)
                {
                    minDistance = distance;
                    wpResult = idLinkPair.second;
                }
            }
        }

        // Return result
        rDistance = minDistance;
        return wpResult;
    }
}


void Tab::ConvertToCEFPixel(double& rWebViewPixelX, double& rWebViewPixelY) const
{
	rWebViewPixelX = (rWebViewPixelX / (double)_upWebView->GetWidth()) * (double)_upWebView->GetResolutionX();
	rWebViewPixelY = (rWebViewPixelY / (double)_upWebView->GetHeight()) * (double)_upWebView->GetResolutionY();
}

void Tab::ConvertToWebViewPixel(double& rCEFPixelX, double& rCEFPixelY) const
{
	rCEFPixelX = (rCEFPixelX / (double)_upWebView->GetResolutionX()) * (float)_upWebView->GetWidth();
	rCEFPixelY = (rCEFPixelY / (float)_upWebView->GetResolutionY()) * (float)_upWebView->GetHeight();
}

void Tab::ReplyJSDialog(bool clickedOk, std::string userInput)
{
	_pCefMediator->ReplyJSDialog(this, clickedOk, userInput);
}

void Tab::PlaySound(std::string filepath)
{
	_pMaster->PlaySound(filepath);
}

std::weak_ptr<CustomTransformationInterface> Tab::GetCustomTransformationInterface() const
{
	return _pMaster->GetCustomTransformationInterface();
}

void Tab::NotifyTextInput(std::string tag, std::string id, int charCount, int charDistance, float x, float y, float duration)
{
	// Tell social record
	if (_spSocialRecord != nullptr)
	{
		_spSocialRecord->AddTextInput(tag, id, charCount, charDistance, x, y, duration);
	}
}