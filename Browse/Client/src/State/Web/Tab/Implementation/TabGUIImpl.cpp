//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/State/Web/Tab/Tab.h"
#include "src/State/Web/Tab/Pipelines/ClickPipeline.h"
#include "src/State/Web/Tab/Pipelines/PivotMenuPipeline.h"
#include "src/State/Web/Tab/Pipelines/TextSelectionPipeline.h"
#include "src/CEF/Mediator.h"
#include "src/Utils/MakeUnique.h"
#include "src/Master/Master.h"

void Tab::TabButtonListener::down(eyegui::Layout* pLayout, std::string id)
{
	if (pLayout == _pTab->_pPanelLayout)
	{
		// ### Tab layout ###
		if (id == "click_mode")
		{
			_pTab->PushBackPipeline(std::move(std::unique_ptr<ClickPipeline>(new ClickPipeline(_pTab))));
		}
		else if (id == "auto_scrolling")
		{
			_pTab->_autoScrolling = true;
		}
		else if (id == "scroll_to_top")
		{
			_pTab->_pCefMediator->ResetScrolling(_pTab);
		}
		else if (id == "zoom")
		{
			_pTab->_zoomLevel = 1.5;

			// Trigger zooming in CefMediator
			_pTab->_pCefMediator->SetZoomLevel(_pTab);
		}
		else if (id == "dashboard")
		{
			auto parameters = _pTab->_pMaster->GetDashboardParameters();
			std::string URL(setup::DASHBOARD_URL + "/?");
			URL += "email=" + parameters.email + "&";
			URL += "pass=" + parameters.password + "&";
			URL += "api_key=" + parameters.APIKey + "&";
			URL += "project_id=" + parameters.projectId;
			_pTab->OpenURL(URL);
		}
		/*
        else if (id == "gaze_mouse")
        {
            _pTab->_gazeMouse = true;
        }
		*/
        else if (id == "selection")
        {
            _pTab->PushBackPipeline(std::make_unique<TextSelectionPipeline>(_pTab));
        }
        /*
        else if (id == "pivot")
        {
			_pTab->PushBackPipeline(std::make_unique<PivotMenuPipeline>(_pTab));
        }
        */
		/*
		else if (id == "test_button") // TODO: only for testing new features
		{
			_pTab->PushBackPipeline(std::make_unique<TextSelectionPipeline>(_pTab));
		}
		*/
	}
	else if(pLayout == _pTab->_pPipelineAbortLayout)
	{
		// ### Pipeline abort layout ###
		if (id == "abort")
		{
			_pTab->AbortAndClearPipelines();
		}
	}
	else if (pLayout == _pTab->_pVideoModeLayout)
	{
		// ### Vide mode layout ###
		if (id == "play")
		{
			auto iter = _pTab->_VideoMap.find(_pTab->_videoModeId);
			if (iter != _pTab->_VideoMap.end()) // search for DOMVideo corresponding to videoModeId
			{
				iter->second->SetPlaying(true);
				eyegui::setVisibilityOfLayout(_pTab->_pVideoModePauseOverlayLayout, false, false, true); // hide pause overlay
			}
		}
		else if (id == "pause")
		{
			auto iter = _pTab->_VideoMap.find(_pTab->_videoModeId);
			if (iter != _pTab->_VideoMap.end()) // search for DOMVideo corresponding to videoModeId
			{
				iter->second->SetPlaying(false);
				eyegui::setVisibilityOfLayout(_pTab->_pVideoModePauseOverlayLayout, true, true, true); // show pause overlay
			}
		}
		else if (id == "volume_up")
		{
			auto iter = _pTab->_VideoMap.find(_pTab->_videoModeId);
			if (iter != _pTab->_VideoMap.end()) // search for DOMVideo corresponding to videoModeId
			{
				iter->second->SetMuted(false);
				iter->second->ChangeVolume(0.25f);
			}
		}
		else if (id == "volume_down")
		{
			auto iter = _pTab->_VideoMap.find(_pTab->_videoModeId);
			if (iter != _pTab->_VideoMap.end()) // search for DOMVideo corresponding to videoModeId
			{
				iter->second->SetMuted(false);
				iter->second->ChangeVolume(-0.25f);
			}
		}
		else if (id == "mute")
		{
			auto iter = _pTab->_VideoMap.find(_pTab->_videoModeId);
			if (iter != _pTab->_VideoMap.end()) // search for DOMVideo corresponding to videoModeId
			{
				iter->second->ToggleMuted();
			}
		}
		else if (id == "exit")
		{
			_pTab->ExitVideoMode();
		}
	}
	else
	{
		// ### Vide mode pause overlay layout ###
		if (id == "skip-10")
		{
			auto iter = _pTab->_VideoMap.find(_pTab->_videoModeId);
			if (iter != _pTab->_VideoMap.end()) // search for DOMVideo corresponding to videoModeId
			{
				iter->second->SkipSeconds(-10);
			}
		}
		if (id == "skip+30")
		{
			auto iter = _pTab->_VideoMap.find(_pTab->_videoModeId);
			if (iter != _pTab->_VideoMap.end()) // search for DOMVideo corresponding to videoModeId
			{
				iter->second->SkipSeconds(30);
			}
		}
	}
}

void Tab::TabButtonListener::up(eyegui::Layout* pLayout, std::string id)
{
	if (pLayout == _pTab->_pPanelLayout)
	{
		// ### Tab layout ###
		if (id == "auto_scrolling")
		{
			_pTab->_autoScrolling = false;
		}
		else if (id == "zoom")
		{
			_pTab->_zoomLevel = 1.0;

			// Trigger zooming in CefMediator
			_pTab->_pCefMediator->SetZoomLevel(_pTab);
		}
        else if (id == "gaze_mouse")
        {
            _pTab->_gazeMouse = false;
            // TODO: reset mouse cursor position?
        }
	}
	else
	{
		// ### Pipeline abort layout ###
	}
}

void Tab::TabSensorListener::penetrated(eyegui::Layout* pLayout, std::string id, float amount)
{
	if (pLayout == _pTab->_pScrollingOverlayLayout)
	{
		if (id == "scroll_up_sensor")
		{
            _pTab->_pCefMediator->EmulateMouseWheelScrolling(_pTab, 0, (amount * 1000.f ) * _pTab->GetLastTimePerFrame());
		}
		else if (id == "scroll_down_sensor")
		{
            _pTab->_pCefMediator->EmulateMouseWheelScrolling(_pTab, 0, (amount * - 1000.f) * _pTab->GetLastTimePerFrame());
		}
	}
}

void Tab::TabOverlayButtonListener::down(eyegui::Layout* pLayout, std::string id)
{
	// Search for id in map
	auto iter = _pTab->_overlayButtonDownCallbacks.find(id);
	if (iter != _pTab->_overlayButtonDownCallbacks.end())
	{
		// Execute callback
		iter->second();
	}
}

void Tab::TabOverlayButtonListener::up(eyegui::Layout* pLayout, std::string id)
{
	// Search for id in map
	auto iter = _pTab->_overlayButtonUpCallbacks.find(id);
	if (iter != _pTab->_overlayButtonUpCallbacks.end())
	{
		// Execute callback
		iter->second();
	}
}

void Tab::TabOverlayButtonListener::selected(eyegui::Layout* pLayout, std::string id)
{
	// Search for id in map
	auto iter = _pTab->_overlayButtonSelectedCallbacks.find(id);
	if (iter != _pTab->_overlayButtonSelectedCallbacks.end())
	{
		// Execute callback
		iter->second();
	}
}

void Tab::TabOverlayKeyboardListener::keySelected(eyegui::Layout* pLayout, std::string id, std::string value)
{
	// Search for id in map
	auto iter = _pTab->_overlayKeyboardSelectCallbacks.find(id);
	if (iter != _pTab->_overlayKeyboardSelectCallbacks.end())
	{
		// Execute callback
		iter->second(value);
	}
}

void Tab::TabOverlayKeyboardListener::keyPressed(eyegui::Layout* pLayout, std::string id, std::u16string value)
{
	// Search for id in map
	auto iter = _pTab->_overlayKeyboardPressCallbacks.find(id);
	if (iter != _pTab->_overlayKeyboardPressCallbacks.end())
	{
		// Execute callback
		iter->second(value);
	}
}

void Tab::TabOverlayWordSuggestListener::chosen(eyegui::Layout* pLayout, std::string id, std::u16string value)
{
	// Search for id in map
	auto iter = _pTab->_overlayWordSuggestCallbacks.find(id);
	if (iter != _pTab->_overlayWordSuggestCallbacks.end())
	{
		// Execute callback
		iter->second(value);
	}
}
