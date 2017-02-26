//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Master is owner of all objects but the CEF implementation which is handed over
// as pointer to the mediator interface at construction. Does window and eyeGUI
// management. Pause overlay is handled completely by Master.

#ifndef MASTER_H_
#define MASTER_H_

#include "src/MasterNotificationInterface.h"
#include "src/Singletons/LabStreamMailer.h"
#include "src/CEF/Mediator.h"
#include "src/State/Web/Web.h"
#include "src/State/Settings/Settings.h"
#include "src/Input/EyeInput.h"
#include "src/Setup.h"
#include "src/Utils/LerpValue.h"
#include "src/Utils/Framebuffer.h"
#include "src/Utils/RenderItem.h"
#include "externals/OGL/gl_core_3_3.h"
#include "submodules/eyeGUI/include/eyeGUI.h"
#include <queue>

// Forward declaration
class Texture;
struct GLFWwindow;

class Master : public MasterNotificationInterface
{
public:

    // Constructor takes pointer to CefMediator
    Master(Mediator* pMediator, std::string userDirectory);

    // Destructor
    virtual ~Master();

    // Run the master which updates CEF
    void Run();

    // Getter for window width and height
    int GetWindowWidth() const { return _width; }
    int GetWindowHeight() const { return _height; }

    // Get time provided by GLFW
    double GetTime() const;

	// Get whether paused
	bool IsPaused() const { return _paused; }

    // Exit
    void Exit();

    // Set gaze visualization (of super GUI)
    void SetGazeVisualization(bool show) { eyegui::setGazeVisualizationDrawing(_pSuperGUI, show); }

    // Set show descriptions (of standard GUI)
    void SetShowDescriptions(bool show)
    {
        eyegui::setDescriptionVisibility(
            _pGUI,
            (show ?
                eyegui::DescriptionVisibility::ON_PENETRATION
                : eyegui::DescriptionVisibility::HIDDEN));
    }

    // Get id of dictionary
    unsigned int GetDictionary() const { return _dictonaryId; }

	// Get user directory location
	std::string GetUserDirectory() const { return _userDirectory; }

	// Push notification to display
	virtual void PushNotification(std::u16string content);

	// Push notification to display taken from localization file
	virtual void PushNotificationByKey(std::string key);

	void RegisterJavascriptCallback(std::string prefix, std::function<void (std::string)>& callbackFunction)
	{
		_pCefMediator->RegisterJavascriptCallback(prefix, callbackFunction);
	};

    // ### EYEGUI DELEGATION ###

    // Add layout to eyeGUI
    eyegui::Layout* AddLayout(std::string filepath, int layer, bool visible);

    // Remove layout from eyeGUI
    void RemoveLayout(eyegui::Layout* pLayout);

	// Fetch localization string by key
	std::u16string FetchLocalization(std::string key) const;

	// Set value of style property in style tree
	void SetStyleTreePropertyValue(std::string styleClass, eyegui::StylePropertyFloat type, std::string value);
	void SetStyleTreePropertyValue(std::string styleClass, eyegui::StylePropertyVec4 type, std::string value);

	// ### SETTINGS ACCESS ###

	// Set homepage URL in settings
	void SetHomepage(std::string URL) { _upSettings->SetHomepage(URL); }


private:

    // Give listener full access
    friend class MasterButtonListener;

    // Listener for GUI
    class MasterButtonListener: public eyegui::ButtonListener
    {
    public:

        MasterButtonListener(Master* pMaster) { _pMaster = pMaster; }
        virtual void hit(eyegui::Layout* pLayout, std::string id) {}
        virtual void down(eyegui::Layout* pLayout, std::string id);
        virtual void up(eyegui::Layout* pLayout, std::string id);

    private:

        Master* _pMaster;
    };

    // Instance of listener
    std::shared_ptr<MasterButtonListener> _spMasterButtonListener;

    // Loop of master
    void Loop();

    // Callbacks
    void GLFWKeyCallback(int key, int scancode, int action, int mods);
    void GLFWMouseButtonCallback(int button, int action, int mods);
    void GLFWCursorPosCallback(double xpos, double ypos);
    void GLFWResizeCallback(int width, int height);
    void GUIResizeCallback(int width, int height);
    void GUIPrintCallback(std::string message) const;

    // States
    std::unique_ptr<Web> _upWeb;
    std::unique_ptr<Settings> _upSettings;

    // GLFW window
    GLFWwindow* _pWindow;

    // Layer between framework and CEF
    Mediator* _pCefMediator;

    // Pointer to eyeGUI
    eyegui::GUI* _pGUI;
    eyegui::GUI* _pSuperGUI; // extra GUI for example for pause overlay since it needs seperate input consumption

    // Time
    double _lastTime;

    // Window resolution
    int _width = setup::INITIAL_WINDOW_WIDTH;
    int _height = setup::INITIAL_WINDOW_HEIGHT;

    // GLFW callback reminder
    bool _leftMouseButtonPressed = false;
    bool _enterKeyPressed = false;

    // Current state
    StateType _currentState;

    // Eye input
    std::unique_ptr<EyeInput> _upEyeInput;

    // Id of dictionary in eyeGUI
    unsigned int _dictonaryId = 0;

    // Time until input is accepted
    float _timeUntilInput = setup::DURATION_BEFORE_INPUT;

    // Layout for pause button etc.
    eyegui::Layout* _pSuperLayout;

    // Emtpy layout to handle cursor floating frame that may not take input
    eyegui::Layout* _pCursorLayout;

    // Floating frame index of cusor
    unsigned int _cursorFrameIndex = 0;

    // Bool to indicate pause (PAUSED_AT_STARTUP used in constructor). Pauses input, not application!
    bool _paused = false;

    // Lerp value to show pause as dimming of whole screen
    LerpValue _pausedDimming;

    // Framebuffer for complete rendering
    std::unique_ptr<Framebuffer> _upFramebuffer;

    // Render item to render screenfilling quad
    std::unique_ptr<RenderItem> _upScreenFillingQuad;

	// Directory for bookmarks etc
	std::string _userDirectory;

	// Frame index of notification message
	unsigned int _notificationFrameIndex = 0;

	// Stack with content for notifications
	std::queue<std::u16string> _notificationStack;

	// LabStreamMailer callback to print incoming messages to log
	std::shared_ptr<LabStreamCallback> _spLabStreamCallback;

	// Time of notification displaying
	float _notificationTime;

	// Boolean to indicate exiting the applicatoin
	bool _exit = false;
};

#endif // MASTER_H_
