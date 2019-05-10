//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Master is owner of all objects but the CEF implementation which is handed over
// as pointer to the mediator interface at construction. Does window and eyeGUI
// management. Pause overlay is handled completely by Master.

#ifndef MASTER_H_
#define MASTER_H_

#include "src/Master/MasterNotificationInterface.h"
#include "src/Master/MasterThreadsafeInterface.h"
#include "src/Master/SensorRecorder.h"
#include "src/Singletons/LabStreamMailer.h"
#include "src/Singletons/FirebaseMailer.h"
#include "src/CEF/Mediator.h"
#include "src/State/Web/Web.h"
#include "src/State/Settings/Settings.h"
#include "src/Input/EyeInput.h"
#include "src/Input/VoiceInput.h"
#include "src/Setup.h"
#include "src/Utils/LerpValue.h"
#include "src/Utils/Framebuffer.h"
#include "src/Utils/RenderItem.h"
#include "src/Input/Filters/CustomTransformationInteface.h"
#include "externals/OGL/gl_core_3_3.h"
#include "submodules/eyeGUI/include/eyeGUI.h"
#include <queue>

// Forward declaration
class Texture;
struct GLFWwindow;

class Master : public MasterNotificationInterface, public MasterThreadsafeInterface
{
public:

	// Constructor takes pointer to CefMediator
	Master(Mediator* pMediator, std::string userDirectory, bool useVoice);

	// Destructor
	virtual ~Master();

	// Run the master which updates CEF. Returns whether computer should shut down
	bool Run();

	// Getter for window width and height
	int GetWindowWidth() const { return _width; }
	int GetWindowHeight() const { return _height; }

	// Getter for screen width and height
	int GetScreenWidth() const;
	int GetScreenHeight() const;

	// Get time provided by GLFW
	double GetTime() const;

	// Get whether paused
	bool IsPaused() const { return _paused; }

	// Exit
	void Exit(bool shutdown = false);

	// Get id of dictionary
	unsigned int GetDictionary() const { return _dictonaryId; }

	// Get user directory location
	std::string GetUserDirectory() const { return _userDirectory; }

	// Register JavaScript callback
	void RegisterJavascriptCallback(std::string prefix, std::function<void(std::string)>& callbackFunction)
	{
		_pCefMediator->RegisterJavascriptCallback(prefix, callbackFunction);
	};

	// Set data transfer
	void SetDataTransfer(bool dataTransfer);

	// Get data transfer policy
	bool MayTransferData() const
	{
		return _dataTransfer;
	}

	// Get pointer to interface of custom transformation of eye input
	std::weak_ptr<CustomTransformationInterface> GetCustomTransformationInterface();

	// Get parameters to log into dashboard
	struct DashboardParameters
	{
		DashboardParameters(std::string email, std::string password, std::string APIKey, std::string projectId) :
			email(email), password(password), APIKey(APIKey), projectId(projectId) {}
		std::string email;
		std::string password;
		std::string APIKey;
		std::string projectId;
	};
	DashboardParameters GetDashboardParameters() const
	{
		return DashboardParameters(_upSettings->GetFirebaseEmail(), _upSettings->GetFirebasePassword(), setup::FIREBASE_API_KEY, setup::FIREBASE_PROJECT_ID);
	}

	// Push back async job. Only provide threadsafe calls to the job!!!
	void PushBackAsyncJob(std::function<bool()> job);
	void SimplePushBackAsyncJob(FirebaseIntegerKey countKey, FirebaseJSONKey recordKey, nlohmann::json record = nlohmann::json()); // automatically adds start index and date

																																   // ### EYEGUI DELEGATION ###

																																   // Add layout to eyeGUI
	eyegui::Layout* AddLayout(std::string filepath, int layer, bool visible);

	// Remove layout from eyeGUI
	void RemoveLayout(eyegui::Layout* pLayout);

	// Fetch localization string by key
	std::u16string FetchLocalization(std::string key) const;

	// Set value of style property in style tree
	void SetStyleTreePropertyValue(std::string styleClass, eyegui::property::Duration type, std::string value);
	void SetStyleTreePropertyValue(std::string styleClass, eyegui::property::Color type, std::string value);

	// Play some sound
	void PlaySound(std::string filepath)
	{
		eyegui::playSound(_pGUI, filepath);
	}

	// Notify about click
	void NotifyClick(std::string tag, std::string id, float x, float y);

	// Use eyeGUI drift map to perform drift correction. Does nothing if USE_EYEGUI_DRIFT_MAP is false
	void ApplyGazeDriftCorrection(float& rPixelX, float& rPixelY) const
	{
		if (_useDriftMap) // only if drift map is used by GazeTheWeb
		{
			eyegui::applyDriftMap(_pGUI, rPixelX, rPixelY);
		}
	}

	// ### ACCESS BY SETTINGS ###

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

	// Set global keyboard layout
	void SetKeyboardLayout(eyegui::KeyboardLayout keyboardLayout)
	{
		// Tell it eyeGUI
		eyegui::setKeyboardLayout(_pGUI, keyboardLayout);
	}

	// Decide whether to block ads
	void BlockAds(bool blockAds) { _pCefMediator->BlockAds(blockAds); }

	// ### STORING OF SETTINGS ###

	// Store homepage URL in settings
	void StoreHomepage(std::string URL) { _upSettings->StoreHomepage(URL); }

	// Set global keyboard layout
	void StoreAndSetKeyboardLayout(eyegui::KeyboardLayout keyboardLayout)
	{
		// Store it in settings (which calls then setter above)
		_upSettings->StoreKeyboardLayout(keyboardLayout);
	}

	// #####################################
	// ### MASTER NOTIFICATION INTERFACE ###
	// #####################################

	// Push notification to display
	virtual void PushNotification(std::u16string content, Type type, bool overridable);

	// Push notification to display taken from localization file
	virtual void PushNotificationByKey(std::string key, Type type, bool overridable);

	// ###################################
	// ### MASTER THREADSAFE INTERFACE ###
	// ###################################

	// Notify about eye tracker status
	virtual void threadsafe_NotifyEyeTrackerStatus(EyeTrackerStatus status, EyeTrackerDevice device);

	// Get whether data may be transferred
	virtual bool threadsafe_MayTransferData();

private:

	// Give listener full access
	friend class MasterButtonListener;

	// Listener for GUI
	class MasterButtonListener : public eyegui::ButtonListener
	{
	public:

		MasterButtonListener(Master* pMaster) { _pMaster = pMaster; }
		virtual void hit(eyegui::Layout* pLayout, std::string id) {}
		virtual void down(eyegui::Layout* pLayout, std::string id);
		virtual void up(eyegui::Layout* pLayout, std::string id);
		virtual void selected(eyegui::Layout* pLayout, std::string id) {}

	private:

		Master* _pMaster;
	};

	// Instance of listener
	std::shared_ptr<MasterButtonListener> _spMasterButtonListener;

	// Notification struct
	struct Notification
	{
		// Constructor
		Notification(std::u16string message, MasterNotificationInterface::Type type, bool overridable, std::string sound = "") : message(message), type(type), overridable(overridable), sound(sound) {}

		// Fields
		std::u16string message;
		MasterNotificationInterface::Type type;
		bool overridable;
		std::string sound;
	};

	// ThreadJob class (class to store a job assigned by a thread)
	class ThreadJob
	{
	public:

		// Constructor
		ThreadJob(Master* pMaster) : _pMaster(pMaster) {}

		// Destructor
		virtual ~ThreadJob() = 0;

		// Execute
		virtual void Execute() = 0;

	protected:

		// Members
		Master* _pMaster;
	};
	class PushEyetrackerStatusThreadJob : public ThreadJob
	{
	public:

		// Constructor
		PushEyetrackerStatusThreadJob(Master* pMaster, EyeTrackerStatus status, EyeTrackerDevice device) : ThreadJob(pMaster), _status(status), _device(device) {};

		// Destructor
		~PushEyetrackerStatusThreadJob() {}

		// Execute
		virtual void Execute();

	private:

		// Members
		EyeTrackerStatus _status;
		EyeTrackerDevice _device;
	};

	// List jobs as friends with benefits
	friend class PushEyetrackerStatusThreadJob;

	// Loop of master
	void Loop();

	// Update async jobs
	void UpdateAsyncJobs(bool wait); // wait indicates that it should block the thread until all async jobs are finished

									 // Show super calibration layout
	void ShowSuperCalibrationLayout();

	// Persist drift grid of drift grid map in Firebase
	enum class PersistDriftGridReason { EXIT, RECALIBRATION, MANUAL };
	void PersistDriftGrid(PersistDriftGridReason reason);

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

	// Voice input
	bool _useVoice = false;
	std::shared_ptr<VoiceInput> _spVoiceInputObject;
	bool _keyboardActive = false;

	// Id of dictionary in eyeGUI
	unsigned int _dictonaryId = 0;

	// Time until input is accepted
	float _timeUntilInput = setup::DURATION_BEFORE_INPUT;

	// Layout for pause button etc.
	eyegui::Layout* _pSuperLayout;

	// Layout to trigger calibration etc.
	eyegui::Layout* _pSuperCalibrationLayout;

	// Layout to display notifications
	eyegui::Layout* _pSuperNotificationLayout;

	// Emtpy layout to handle cursor floating frame that may not take input
	eyegui::Layout* _pCursorLayout;

	// Floating frame index of cursor
	unsigned int _cursorFrameIndex = 0;

	// Floating frame indices for eyes in trackbox display of super calibration layout
	unsigned int _trackboxLeftFrameIndex = 0;
	unsigned int _trackboxRightFrameIndex = 0;

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

	// Queue with upcoming notifications
	std::queue<Notification> _notificationStack;

	// Time of notification displaying
	float _notificationTime = 0.f;

	// Whether current notification is overridable or not
	bool _notificationOverridable = false;

	// LabStreamMailer callback to print incoming messages to log
	std::shared_ptr<LabStreamCallback> _spLabStreamCallback;

	// Boolean to indicate exiting the application
	bool _exit = false;

	// Buffer for jobs assigned by threads
	std::vector<std::shared_ptr<ThreadJob> > _threadJobs;

	// Mutex to guarantees that threadJobs are not manipulated while read by master
	std::mutex _threadJobsMutex;

	// Bool to control data transfer (set by Web as there is the placed the button)
	bool _dataTransfer = true;

	// Asyncronous calls, e.g. persist Firebase entries
	std::vector<std::future<bool> > _asyncJobs; // abuse async calls since it is easier to determine whether finished or not

												// Indicator whether computer should shut down at exit
	bool _shouldShutdownAtExit = false;

	// Last calibration points
	std::vector<int> _lastCalibrationPointsFrameIndices;

	// Sensor recording
	SensorRecorder _sensorRecorder;

	// Store whether drift map is used
	bool _useDriftMap = false;

	// Duration super calibration layout is visible (after long time, shut down the system)
	double _recalibrationLayoutTime = 0.0;

	// Monitor resolution
	int _monitorWidth = -1;
	int _monitorHeight = -1;

	// Bool to indicate demo mode reset which is handled in the update loop
	bool _demoModeReset = false;
};

#endif // MASTER_H_
