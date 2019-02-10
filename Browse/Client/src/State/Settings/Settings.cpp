//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "Settings.h"
#include "src/Master/Master.h"
#include "src/Global.h"
#include "src/Utils/Logger.h"
#include "submodules/eyeGUI/externals/TinyXML2/tinyxml2.h"

// Include singleton for mailing to JavaScript
#include "src/Singletons/JSMailer.h"

Settings::Settings(Master* pMaster) : State(pMaster)
{
	// Initialize members
	_fullpathSettings = pMaster->GetUserDirectory() + SETTINGS_FILE;

	// Load settings and apply them
	bool saveSettings = false;
	if (!LoadSettings())
	{
		LogInfo("Settings: No settings file found or parsing error");
		saveSettings = true;
	}
	ApplySettings(saveSettings); // sets settings

	// Create layouts
	_pSettingsLayout = _pMaster->AddLayout("layouts/Settings.xeyegui", EYEGUI_SETTINGS_LAYER, false);
	_pGeneralLayout = _pMaster->AddLayout("layouts/SettingsGeneral.xeyegui", EYEGUI_SETTINGS_LAYER, false);
	_pAdBlockingLayout = _pMaster->AddLayout("layouts/SettingsAdBlocking.xeyegui", EYEGUI_SETTINGS_LAYER, false);
	_pInfoLayout = _pMaster->AddLayout("layouts/SettingsInfo.xeyegui", EYEGUI_SETTINGS_LAYER, false);

    // Set state of switches before registering listerners to avoid unnecessary saving
    if(_globalSetup.showDescriptions) { eyegui::buttonDown(_pGeneralLayout, "toggle_descriptions", true); }
    if(_globalSetup.showGazeVisualization) { eyegui::buttonDown(_pGeneralLayout, "toggle_gaze_visualization", true); }
	if(_globalSetup.adBlocking) { eyegui::buttonDown(_pAdBlockingLayout, "toggle_ad_blocking", true); };

	// Button listener
	_spSettingsButtonListener = std::shared_ptr<SettingsButtonListener>(new SettingsButtonListener(this));
	eyegui::registerButtonListener(_pSettingsLayout, "close", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pSettingsLayout, "general", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pSettingsLayout, "ad_blocking", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pSettingsLayout, "info", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pSettingsLayout, "shutdown", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pGeneralLayout, "toggle_descriptions", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pGeneralLayout, "toggle_gaze_visualization", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pGeneralLayout, "back", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pAdBlockingLayout, "toggle_ad_blocking", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pAdBlockingLayout, "back", _spSettingsButtonListener);
	eyegui::registerButtonListener(_pInfoLayout, "back", _spSettingsButtonListener);

	// Deactivate buttons which are not used, yet
	eyegui::setElementActivity(_pSettingsLayout, "input", false, false);

	// Deactivate shutdown button when in demo mode
	if (setup::DEMO_MODE)
	{
		eyegui::setElementActivity(_pSettingsLayout, "shutdown", false, false);
	}
}

Settings::~Settings()
{
	// Delete layouts?
}

StateType Settings::Update(float tpf, const std::shared_ptr<const Input> spInput, std::shared_ptr<VoiceAction> spVoiceInput, bool *keyboardActive)
{
    if (_goToWeb)
	{
		return StateType::WEB;
	}
	else
	{
		return StateType::SETTINGS;
	}
}

void Settings::Draw() const
{

}

void Settings::Activate()
{
	// Super
	State::Activate();

	// Make layout visible
	eyegui::setVisibilityOfLayout(_pSettingsLayout, true, true, true);

	// Reset stuff
	_goToWeb = false;
}

void Settings::Deactivate()
{
	// Super
	State::Deactivate();

	// Make all layouts invisible
	eyegui::setVisibilityOfLayout(_pSettingsLayout, false, false, true);
}

// Save settings to hard disk. Returns whether successful
bool Settings::SaveSettings() const
{
	// Create document
	tinyxml2::XMLDocument doc;

	// Create and insert root
	tinyxml2::XMLNode* pRoot = doc.NewElement("settings");
	doc.InsertFirstChild(pRoot);

	// Create environment for global setup
	tinyxml2::XMLNode* pGlobal = doc.NewElement("global");
	pRoot->InsertFirstChild(pGlobal);

	// Descriptions
	tinyxml2::XMLElement* pDescriptions = doc.NewElement("descriptions");
	pDescriptions->SetAttribute("visible", _globalSetup.showDescriptions ? "true" : "false");
	pGlobal->InsertFirstChild(pDescriptions);

	// Gaze visualization
	tinyxml2::XMLElement* pGazeVisualization = doc.NewElement("gazevisualization");
	pGazeVisualization->SetAttribute("visible", _globalSetup.showGazeVisualization ? "true" : "false");
	pGlobal->InsertAfterChild(pDescriptions, pGazeVisualization);

	// Ad blocking
	tinyxml2::XMLElement* pAdBlocking = doc.NewElement("adblocking");
	pAdBlocking->SetAttribute("block", _globalSetup.adBlocking ? "true" : "false");
	pGlobal->InsertAfterChild(pGazeVisualization, pAdBlocking);

	// Keyboard layout
	tinyxml2::XMLElement* pKeyboardLayout = doc.NewElement("keyboardlayout");
	std::string layout;
	switch (_globalSetup.keyboardLayout)
	{
	case eyegui::KeyboardLayout::US_ENGLISH:
		layout = "US_ENGLISH";
		break;
	case eyegui::KeyboardLayout::GERMANY_GERMAN:
		layout = "GERMANY_GERMAN";
		break;
	case eyegui::KeyboardLayout::ISRAEL_HEBREW:
		layout = "ISRAEL_HEBREW";
		break;
	case eyegui::KeyboardLayout::GREECE_GREEK:
		layout = "GREECE_GREEK";
		break;
	}
	pKeyboardLayout->SetAttribute("layout", layout.c_str());
	pGlobal->InsertAfterChild(pAdBlocking, pKeyboardLayout);

	// Firebase email and password
	tinyxml2::XMLElement* pFirebase = doc.NewElement("firebase");
	pFirebase->SetAttribute("email", _globalSetup.firebaseEmail.c_str());
	pFirebase->SetAttribute("password", _globalSetup.firebasePassword.c_str());
	pGlobal->InsertFirstChild(pFirebase);

	// Eyetracker geometry
	tinyxml2::XMLElement* pGeometry = doc.NewElement("eyetrackergeometry");
	pGeometry->SetAttribute("monitorWidth", _globalSetup.eyetrackerGeometry.monitorWidth);
	pGeometry->SetAttribute("monitorHeight", _globalSetup.eyetrackerGeometry.monitorHeight);
	pGeometry->SetAttribute("mountingAngle", _globalSetup.eyetrackerGeometry.mountingAngle);
	pGeometry->SetAttribute("relativeDistanceHeight", _globalSetup.eyetrackerGeometry.relativeDistanceHeight);
	pGeometry->SetAttribute("relativeDistanceDepth", _globalSetup.eyetrackerGeometry.relativeDistanceDepth);
	pGlobal->InsertFirstChild(pGeometry);

	// Create environment for web setup
	tinyxml2::XMLNode* pWeb = doc.NewElement("web");
	pRoot->InsertAfterChild(pGlobal, pWeb);

	// Homepage
	tinyxml2::XMLElement* pHomepage = doc.NewElement("homepage");
	pHomepage->SetAttribute("url", _webSetup.homepage.c_str());
	pWeb->InsertFirstChild(pHomepage);
	
	// Try to save file
	tinyxml2::XMLError result = doc.SaveFile(_fullpathSettings.c_str());

	// Return whether successful
	return (result == tinyxml2::XMLError::XML_SUCCESS);
}

void Settings::ApplySettings(bool save)
{
	// Global
	_pMaster->SetShowDescriptions(_globalSetup.showDescriptions);
	_pMaster->SetGazeVisualization(_globalSetup.showGazeVisualization);
	_pMaster->SetKeyboardLayout(_globalSetup.keyboardLayout);
	_pMaster->BlockAds(_globalSetup.adBlocking);

	// Save it
	if (save)
	{
		if (!SaveSettings()) { LogInfo("Settings: Failed to save settings"); }
	}
}

// Load settings from hard disk. Returns whether successful
bool Settings::LoadSettings()
{
	// Open document
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError result = doc.LoadFile(_fullpathSettings.c_str());

	// Check result of opening 
	if (result != tinyxml2::XMLError::XML_SUCCESS) { return false; }

	// Fetch root
	tinyxml2::XMLNode* pRoot = doc.FirstChild();
	if (pRoot == NULL) { return false; }

	// Fetch global
	tinyxml2::XMLNode* pGlobal = pRoot->FirstChild();
	if (pGlobal == NULL) { return false; }

	// Descriptions
	tinyxml2::XMLElement* pDescriptions = pGlobal->FirstChildElement("descriptions");
	if (pDescriptions != NULL)
	{
		_globalSetup.showDescriptions = pDescriptions->BoolAttribute("visible");
	}

	// Gaze visualization
	tinyxml2::XMLElement* pGazeVisualization = pGlobal->FirstChildElement("gazevisualization");
	if (pGazeVisualization != NULL)
	{
		_globalSetup.showGazeVisualization = pGazeVisualization->BoolAttribute("visible");
	}

	// Ad blocking
	tinyxml2::XMLElement* pAdBlocking = pGlobal->FirstChildElement("adblocking");
	if (pAdBlocking != NULL)
	{
		_globalSetup.adBlocking = pAdBlocking->BoolAttribute("block");
	}

	// Keyboard layout
	tinyxml2::XMLElement* pKeyboardLayout = pGlobal->FirstChildElement("keyboardlayout");
	if (pKeyboardLayout != NULL)
	{
		std::string attribute = pKeyboardLayout->Attribute("layout");
		if (attribute == "GERMANY_GERMAN")
		{
			_globalSetup.keyboardLayout = eyegui::KeyboardLayout::GERMANY_GERMAN;
		}
		else if (attribute == "ISRAEL_HEBREW")
		{
			_globalSetup.keyboardLayout = eyegui::KeyboardLayout::ISRAEL_HEBREW;
		}
		else if (attribute == "GREECE_GREEK")
		{
			_globalSetup.keyboardLayout = eyegui::KeyboardLayout::GREECE_GREEK;
		}
		else // "US_ENGLISH" as fallback if string is different
		{
			_globalSetup.keyboardLayout = eyegui::KeyboardLayout::US_ENGLISH;
		}
	}

	// Firebase email and password
	tinyxml2::XMLElement* pFirebase = pGlobal->FirstChildElement("firebase");
	if (pFirebase != NULL)
	{
		_globalSetup.firebaseEmail = pFirebase->Attribute("email");
		_globalSetup.firebasePassword = pFirebase->Attribute("password");
	}

	// Eyetracker geometry
	tinyxml2::XMLElement* pGeometry = pGlobal->FirstChildElement("eyetrackergeometry");
	if (pGeometry != NULL)
	{
		_globalSetup.eyetrackerGeometry.monitorWidth = pGeometry->IntAttribute("monitorWidth");
		_globalSetup.eyetrackerGeometry.monitorHeight = pGeometry->IntAttribute("monitorHeight");
		_globalSetup.eyetrackerGeometry.mountingAngle = pGeometry->IntAttribute("mountingAngle");
		_globalSetup.eyetrackerGeometry.relativeDistanceHeight = pGeometry->IntAttribute("relativeDistanceHeight");
		_globalSetup.eyetrackerGeometry.relativeDistanceDepth = pGeometry->IntAttribute("relativeDistanceDepth");
	}

	// Fetch web setup
	tinyxml2::XMLNode* pWeb = pGlobal->NextSibling();
	if (pWeb == NULL) { return false; }

	// Homepage
	tinyxml2::XMLElement* pHomepage = pWeb->FirstChildElement("homepage");
	if (pHomepage != NULL)
	{
		const char * pURL = NULL;
		pURL = pHomepage->Attribute("url");
		if (pURL != NULL)
		{
			_webSetup.homepage = std::string(pURL);
		}
	}

	// When you came to here no real errors occured
	return true;
}

void Settings::SettingsButtonListener::down(eyegui::Layout* pLayout, std::string id)
{
	if (pLayout == _pSettings->_pSettingsLayout)
	{
		// ### Settings layout ###
		if (id == "close")
		{
			_pSettings->_goToWeb = true;
			JSMailer::instance().Send("close");
		}
		else if (id == "general")
		{
			eyegui::setVisibilityOfLayout(_pSettings->_pGeneralLayout, true, true, true);
			JSMailer::instance().Send("general");
		}
		else if (id == "ad_blocking")
		{
			eyegui::setVisibilityOfLayout(_pSettings->_pAdBlockingLayout, true, true, true);
		}
		else if (id == "info")
		{
			eyegui::setVisibilityOfLayout(_pSettings->_pInfoLayout, true, true, true);
		}
		else if (id == "shutdown")
		{
			_pSettings->_pMaster->Exit(true);
		}
	}
	else if (pLayout == _pSettings->_pGeneralLayout)
	{
		// ### General layout ###
		if (id == "back")
		{
			// Apply and save (TODO: placed here at back because direct toggle_descriptions overrides "PAUSED_AT_STARTUP",
			// as buttons are virtually pressed by constructor, which is stored until first update of eyeGUI.
			// This would cause the description setting to be overriden at first eyeGUI update.)
			_pSettings->ApplySettings(true);
			eyegui::setVisibilityOfLayout(_pSettings->_pGeneralLayout, false, false, true);
		}
		else if (id == "toggle_descriptions")
		{
            _pSettings->_globalSetup.showDescriptions = true;

			// Do notification if layout visible to user
			if(eyegui::isLayoutVisible(_pSettings->_pGeneralLayout))
			{
				_pSettings->_pMaster->PushNotificationByKey("notification:settings:description_on", MasterNotificationInterface::Type::NEUTRAL, true);
			}
		}
		else if (id == "toggle_gaze_visualization")
		{
            _pSettings->_globalSetup.showGazeVisualization = true;
			JSMailer::instance().Send("gaze_on");

			// Do notification if layout visible to user
			if (eyegui::isLayoutVisible(_pSettings->_pGeneralLayout))
			{
				_pSettings->_pMaster->PushNotificationByKey("notification:settings:gaze_visualization_on", MasterNotificationInterface::Type::NEUTRAL, true);
			}
		}
	}
	else if (pLayout == _pSettings->_pAdBlockingLayout)
	{
		// ### Ad blocking layout ###
		if (id == "back")
		{
			_pSettings->ApplySettings(true);
			eyegui::setVisibilityOfLayout(_pSettings->_pAdBlockingLayout, false, false, true);
		}
		else if (id == "toggle_ad_blocking")
		{
			_pSettings->_globalSetup.adBlocking = true;

			// Do notification if layout visible to user
			if (eyegui::isLayoutVisible(_pSettings->_pAdBlockingLayout))
			{
				_pSettings->_pMaster->PushNotificationByKey("notification:settings:ad_blocking_on", MasterNotificationInterface::Type::NEUTRAL, true);
			}
		}
	}
	else if (pLayout == _pSettings->_pInfoLayout)
	{
		// ### Information layout ###
		if (id == "back")
		{
			eyegui::setVisibilityOfLayout(_pSettings->_pInfoLayout, false, false, true);
		}
	}
}

void Settings::SettingsButtonListener::up(eyegui::Layout* pLayout, std::string id)
{
    if (pLayout == _pSettings->_pGeneralLayout)
    {
        // ### General layout ###
        if (id == "toggle_descriptions")
        {
            _pSettings->_globalSetup.showDescriptions = false;

			// Do notification if layout visible to user
			if (eyegui::isLayoutVisible(_pSettings->_pGeneralLayout))
			{
				_pSettings->_pMaster->PushNotificationByKey("notification:settings:description_off", MasterNotificationInterface::Type::NEUTRAL, true);
			}
        }
        else if (id == "toggle_gaze_visualization")
        {
            _pSettings->_globalSetup.showGazeVisualization = false;
			JSMailer::instance().Send("gaze_off");

			// Do notification if layout visible to user
			if (eyegui::isLayoutVisible(_pSettings->_pGeneralLayout))
			{
				_pSettings->_pMaster->PushNotificationByKey("notification:settings:gaze_visualization_off", MasterNotificationInterface::Type::NEUTRAL, true);
			}
        }
    }
	else if (pLayout == _pSettings->_pAdBlockingLayout)
	{
		// ### Ad blocking layout ###
		if (id == "toggle_ad_blocking")
		{
			_pSettings->_globalSetup.adBlocking = false;

			// Do notification if layout visible to user
			if (eyegui::isLayoutVisible(_pSettings->_pAdBlockingLayout))
			{
				_pSettings->_pMaster->PushNotificationByKey("notification:settings:ad_blocking_off", MasterNotificationInterface::Type::NEUTRAL, true);
			}
		}
	}
}
