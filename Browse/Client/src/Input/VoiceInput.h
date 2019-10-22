//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//		   Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================
// Stub for voice input handling.


#ifndef VOICEINPUT_H_
#define VOICEINPUT_H_

#include <atomic>
#include <thread>
#include <memory>
#include <deque>
#include <queue>
#include <mutex>
#include <vector>
#include <codecvt>

#include "go-speech-recognition.h"


enum class VoiceMode {
	COMMAND,	// change mode to COMMAND to use the transcription mode "command and search" (is default, use for browsing)
	FREE		// change mode to COMMAND to use the transcription mode "default" (use for i.e. text input)
};

enum class VoiceInputState {
	Inactive,	// VoiceInput hasn't started/shouldn't be running
	Active,		// VoiceInput runs
	Changing,	// VoiceInput is changing the VoiceMode
	Restarting	// VoiceInput is in the process of deactivation and activation
};

enum class VoiceCommand {

	NO_ACTION,
	SCROLL_UP,		// scroll the page up
	SCROLL_DOWN, 	// scroll the page down
	TOP,			// scroll to the top
	BOTTOM,			// scroll to the bottom
	BOOKMARK,		// bookmark the current page/tab
	BACK,			// return to the last visited page
	REFRESH,		// return to the last visited page
	FORWARD,		// reverse the back command
	GO_TO,			// open url [parameter]
	NEW_TAB,		// create a new tab and open url [parameter]
	SEARCH,			// search for the [parameter] on the current page
	ZOOM,			// toggle zoom on the current page
	TAB_OVERVIEW,	// open the tab overview
	SHOW_BOOKMARKS,	// shows the list of bookmarks
	CLICK,			// click on the nearest clickable element containing the [parameter]
	CHECK,			// check the nearest checkbox
	VIDEO_INPUT,	// open videomode with nearest video
	INCREASE,		// increases sound volume
	DECREASE,		// decreases sound volume
	PLAY,			// play the current video
	PAUSE,			// pause the current video
	STOP,			// stop the current video
	MUTE,			// mute the videos sound
	UNMUTE,			// unmute the videos sound
	TEXT,			// activate text input ?[parameter]
	REMOVE,			// remove last inputted word
	CLEAR,			// clear the entire text input field
	SUBMIT,			// submits inputted text
	CLOSE,			// close the current view and return to the page view
	QUIT,			// quit the browser
	PARAMETER_ONLY	// use when only the parameter is needed i.e. when using text input
};

// Needed to assign Transcripts to VoiceCommands
struct CommandStruct {

	// The command itself
	VoiceCommand command;

	// Possible string representations of the command
	std::vector<std::string> phoneticVariants;

	// Indicates if the command takes parameter
	bool takesParameter;

	// Usable in keyboard mode
	bool usableInFree;

	/*
	Needed to assign Transcripts to VoiceCommands

	Parameter:
	VoiceCommand - a command from the VoiceCommand enum
	phoneticVariants - a vector of strings triggering the VoiceCommand (for logging purpose: first should be the intended key word)
	takesParameter - indicates if the VoiceCommand should take a parameter
	*/
	CommandStruct(VoiceCommand command, std::vector<std::string> phoneticVariants, bool takesParameter, bool usableInFree) {
		this->command = command;
		this->phoneticVariants = phoneticVariants;
		this->takesParameter = takesParameter;
		this->usableInFree = usableInFree;
	}
};


// VoiceCommand: Action(Parameter)
struct VoiceAction {

	VoiceCommand command;
	std::string parameter;

	VoiceAction(VoiceCommand command, std::string parameter) {
		this->command = command;
		this->parameter = parameter;
	}
};


//! Class for holding continuous audio records.
class ContinuousAudioRecord
{
public:
	//! Constructor.
	ContinuousAudioRecord(
		unsigned int channelCount,
		unsigned int sampleRate,
		unsigned int maxSeconds)
		: _maximumSize(channelCount * sampleRate * maxSeconds),
		_channelCount(channelCount),
		_sampleRate(sampleRate),
		_maxSeconds(maxSeconds)
	{
	}

	//! Getter.
	int GetChannelCount() const { return _channelCount; };
	int GetSampleRate() const { return _sampleRate; };
	int GetSampleCount() const { return _index; }

	//! Add sample. Increments index and returns whether successful.
	bool AddSample(short sample);
	std::unique_ptr<std::vector<short> > MoveBuffer();


private:
	//! Members.
	std::deque<short> _buffer;
	const int _maximumSize;
	bool _maximumSizeReached = false;
	mutable std::mutex _bufferGuard;
	std::shared_ptr<std::vector<short> > _spBuffer;
	unsigned int _channelCount;
	unsigned int _sampleRate;
	unsigned int _maxSeconds;
	unsigned int _index = 0;
};

// Class that handles recording and transcribing audio 
class VoiceInput : public std::enable_shared_from_this<VoiceInput>
{

public:

	// Constructor
	VoiceInput(bool allowRestart, bool &finished);

	// Destructor
	virtual ~VoiceInput();

	// Change language of transcribed audio
	void SetLanguage(char* lang) { _language = lang; }

	// Change transcription model [Can be either "video", "phone_call", "command_and_search", "default" (see https://cloud.google.com/speech-to-text/docs/basics)]
	void SetModel(char* model) { _model = model; }

	// Set how many alternatives are received
	void SetMaxAlternatives(int maxAlternatives) { _maxAlternatives = maxAlternatives; }

	// Sets if interim results should be received
	void SetInterimResults(GO_SPEECH_RECOGNITION_BOOL interimResults) { _interimResults = interimResults; }

	// Update voice input
	std::shared_ptr<VoiceAction> Update(float tpf, bool keyboardActive);

	// Start recording and transcribing process.
	void Activate();

	// Stop recording and transcribing process.
	void Deactivate();

	// Changes transcription mode/model
	// input: VoiceMode
	void SetVoiceMode(VoiceMode voiceMode);

	VoiceMode GetVoiceMode() { return _voiceMode; }

	// Returns the how many pixel should be scrolled when using SCROLL_UP/SCROLL_DOWN
	static double GetScrollDistance() { return double(350); }

	VoiceInputState GetState() { return _voiceInputState; }

	// seconds
	std::chrono::seconds GetRunTimeLimit() { return _runTimeLimit; }

	void Reactivate();

	/* 
		Just forwards a shared pointer of itself to the Monitor 
		(is it better if the Master can do this directly?)	
	*/
	void SendVoiceInputToVoiceMonitor(std::shared_ptr<VoiceInput> spVoiceInput);
	
private:

	// Returns whether the plugin itself and all the respective functions are loaded.
	bool IsPluginLoaded();

	GO_SPEECH_RECOGNITION_INITIALIZE_STREAM	GO_SPEECH_RECOGNITION_InitializeStream;
	GO_SPEECH_RECOGNITION_SEND_AUDIO GO_SPEECH_RECOGNITION_SendAudio;
	GO_SPEECH_RECOGNITION_RECEIVE_TRANSCRIPT GO_SPEECH_RECOGNITION_ReceiveTranscript;
	GO_SPEECH_RECOGNITION_GET_LOG GO_SPEECH_RECOGNITION_GetLog;
	GO_SPEECH_RECOGNITION_CLOSE_STREAM GO_SPEECH_RECOGNITION_CloseStream;
	GO_SPEECH_RECOGNITION_IS_INITIALIZED GO_SPEECH_RECOGNITION_IsInitialized;

	// [RECORDING]

	static const int AUDIO_INPUT_SAMPLE_RATE = 16000;
	static const int AUDIO_INPUT_CHANNEL_COUNT = 1;
	static const unsigned int AUDIO_INPUT_MAX_INPUT_SECONDS = 3;

	/*
	Thread variable:
	Read in: main, _tStopping
	Manipulated in: main
	*/
	std::atomic<bool> _portAudioInitialized = false;

	/*
	Thread variable:
	Read in:  _tSending
	Manipulated in: main
	*/
	std::shared_ptr<ContinuousAudioRecord> _spAudioInput;

	// Time
	// To track overall sending
	std::chrono::steady_clock::time_point _startTime;

	// To track sending since last reactivation
	std::chrono::steady_clock::time_point _activationTime;

	std::chrono::seconds _runTimeLimit = std::chrono::seconds(50);

	bool _allowRestart = false;

	// [STREAMING]

	// language to be transcribed
	char* _language = "en-US";

	// _sample_rate of the audio
	int _sampleRate = 16000;

	// transcription model to be used
	char* _model = "command_and_search";

	// how many alternatives are received
	int _maxAlternatives = 3;

	// if you want to get interim results
	GO_SPEECH_RECOGNITION_BOOL _interimResults = GO_SPEECH_RECOGNITION_FALSE;

	// time to query audio in ms
	int _queryTime = 1000;

	// indicating the current state
	VoiceInputState _voiceInputState = VoiceInputState::Inactive;

	std::chrono::steady_clock::time_point _pauseTime;
	std::chrono::seconds _pausedTime = std::chrono::seconds(0);

	/*
	state showing if recording and transcribing should be stopped (use to ensure you are only stopping once at the time)

	Thread variable:
	Read in: _tSending, _tReceiving
	Manipulated in: _tStopping
	*/
	std::atomic<bool> _stopping = true;

	// thread handling the sending
	std::unique_ptr<std::thread> _tSending = nullptr;

	/*
	Thread variable:
	Read in: _tStopping
	Manipulated in: _tSending
	*/
	std::atomic<bool> _isSending = false;

	// thread handling the receving
	std::unique_ptr<std::thread> _tReceiving = nullptr;

	/*
	Thread variable:
	Read in: _tStopping
	Manipulated in: _tReceiving
	*/
	std::atomic<bool> _isReceiving = false;

	// thread handling the reactivating (needed to prevent stopping the whole program)
	std::unique_ptr<std::thread> _tReactivating = nullptr;
	// only one restart at the time
	mutable std::mutex _reactivateGuard;



	// [PROCESSING TRANSCRIPT]

	/*
	last recognized speech as text

	Thread variable (access secured by mutex):
	Read in: main
	Manipulated in: main, _tReceiving
	*/
	std::queue<std::string> _recognitionResults;

	VoiceMode _voiceMode = VoiceMode::COMMAND;

	// protects access to recognition_results
	std::mutex _transcriptGuard;

	// utf8 to utf16
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> _converter;
};

#endif // VOICEINPUT_H_
