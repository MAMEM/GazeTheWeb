//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Settings State.

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "src/State/State.h"
#include "plugins/Eyetracker/Interface/EyetrackerGeometry.h"

class Settings : public State
{
public:

    // Constructor
    Settings(Master* pMaster);

    // Destructor
    virtual ~Settings();

    // #############
    // ### STATE ###
    // #############

    // Update. Returns which state should be active in next time step
    virtual StateType Update(float tpf, const std::shared_ptr<const Input> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject);

    // Draw
    virtual void Draw() const;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

	// Get homepage URL
	std::string GetHomepage() const { return _webSetup.homepage; }

	// Get Firebase Email
	std::string GetFirebaseEmail() const { return _globalSetup.firebaseEmail; }

	// Get Firebase Password
	std::string GetFirebasePassword() const { return _globalSetup.firebasePassword; }

	// Get eyetracker geometry
	EyetrackerGeometry GetEyetrackerGeometry() const { return _globalSetup.eyetrackerGeometry; }

	// Store homepage URL
	void StoreHomepage(std::string URL) { _webSetup.homepage = URL; ApplySettings(true); }

	// Store global keyboard layout
	void StoreKeyboardLayout(eyegui::KeyboardLayout keyboardLayout) { _globalSetup.keyboardLayout = keyboardLayout, ApplySettings(true); }

private:

	// Apply and maybe save settings
	void ApplySettings(bool save = true);

	// Save settings to hard disk. Returns whether successful
	bool SaveSettings() const;

	// Load settings from hard disk. Returns whether successful
	bool LoadSettings();

    // Give listener full access
    friend class SettingsButtonListener;

    // Listener for GUI
    class SettingsButtonListener : public eyegui::ButtonListener
    {
    public:

        SettingsButtonListener(Settings* pSettings) { _pSettings = pSettings; }
        virtual void hit(eyegui::Layout* pLayout, std::string id) {}
        virtual void down(eyegui::Layout* pLayout, std::string id);
        virtual void up(eyegui::Layout* pLayout, std::string id);
		virtual void selected(eyegui::Layout* pLayout, std::string id) {}

    private:

        Settings* _pSettings;
    };

    // Instance of listener
    std::shared_ptr<SettingsButtonListener> _spSettingsButtonListener;

    // Setup of global settings
    class GlobalSetup
    {
    public:

        bool showDescriptions = true;
        bool showGazeVisualization = false;
		bool adBlocking = true;
		eyegui::KeyboardLayout keyboardLayout = eyegui::KeyboardLayout::US_ENGLISH;
		std::string firebaseEmail = "";
		std::string firebasePassword = "";
		EyetrackerGeometry eyetrackerGeometry;
    };

	// Setup of web settings
	class WebSetup
	{
	public:

		std::string homepage = "https://duckduckgo.com";
	};

    // Layouts
    eyegui::Layout* _pSettingsLayout;
    eyegui::Layout* _pGeneralLayout;
	eyegui::Layout* _pAdBlockingLayout;
    eyegui::Layout* _pInfoLayout;

    // Bool to remember whether to switch to Web in next frame
    bool _goToWeb = false;

    // Setups
	GlobalSetup _globalSetup;
	WebSetup _webSetup;

	// Fullpath to settings file
	std::string _fullpathSettings;
};

#endif // SETTINGS_H_
