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

#include "go-speech-recognition.h"

enum class VoiceMode {
	COMMAND,	// change mode to COMMAND to use the transcription mode "command and search" (is default, use for browsing)
	FREE		// change mode to COMMAND to use the transcription mode "default" (use for i.e. text input)
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


	/*
	Needed to assign Transcripts to VoiceCommands

		Parameter:
			VoiceCommand - a command from the VoiceCommand enum
			phoneticVariants - a vector of strings triggering the VoiceCommand (for logging purpose: first should be the intended key word)
			takesParameter - indicates if the VoiceCommand should take a parameter
	*/
	CommandStruct(VoiceCommand command, std::vector<std::string> phoneticVariants, bool takesParameter) {
		this->command = command;
		this->phoneticVariants = phoneticVariants;
		this->takesParameter = takesParameter;
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
class VoiceInput
{

public:

	// Constructor
	VoiceInput();

	// Destructor
	virtual ~VoiceInput();

	// Change language of transcribed audio
	void SetLanguage(char* lang) { _language = lang; }

	// Change transcription model [Can be either "video", "phone_call", "command_and_search", "default" (see https://cloud.google.com/speech-to-text/docs/basics)]
	void SetModel(char* model) { _model = model; }

	// Update voice input
	std::shared_ptr<VoiceAction> Update(float tpf);

	// Start recording and transcribing process.
	void Activate();

	// Stop recording and transcribing process.
	void Deactivate();

	// Returns whether the recording and transcribing process is currently running.
	bool IsActive() { return _active; }

	// Returns whether the stopping process was initialized.
	// The process is really deactivated if IsActive() == false && IsStopped() == true
	bool IsStopping() { return _stopping; }

	// Changes transcription mode/model
	// input: VoiceMode
	void SetVoiceMode(VoiceMode voiceMode);

	VoiceMode GetVoiceMode() { return _voiceMode; }

	// Returns the how many pixel should be scrolled when using SCROLL_UP/SCROLL_DOWN
	static double GetScrollDistance() { return double(350); }	

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

// [STREAMING]

	// language to be transcribed
	char* _language = "en-US";

	// _sample_rate of the audio
	int _sampleRate = 16000;

	// transcription model to be used
	char* _model = "command_and_search";

	// time to query audio in ms
	int _queryTime = 1000;


	/*
	state showing if the transcribing is activate (recording and streaming to google)
	
		Thread variable:
			Read in: main
			Manipulated in: main, _tReceiving
	*/
	std::atomic<bool> _active = false;

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



	// thread handling the stopping (needed to prevent stopping the whole program)
	std::unique_ptr<std::thread> _tStopping = nullptr;


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
};

#endif // VOICEINPUT_H_
