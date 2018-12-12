//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//		   Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================
// Stub for voice input handling.


#ifndef VOICEINPUT_H_
#define VOICEINPUT_H_

#include <thread>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <deque>
#include <atomic>

#include "go-speech-recognition.h"


enum class VoiceCommand
{

	NO_ACTION,
	SCROLL_UP,		// scroll the page up
	SCROLL_DOWN, 	// scroll the page down
	TOP,			// scroll to the top
	BOTTOM,			// scroll to the bottom
	BOOKMARK,		// bookmark the current page/tab
	BACK,			// return to the last visited page
	RELOAD,			// return to the last visited page
	FORWARD,		// reverse the back command
	GO_TO,			// open url [parameter]
	NEW_TAB,		// create a new tab and open url [parameter]
	SEARCH,			// search for the [parameter] on the current page
	ZOOM,			// toggle zoom on the current page
	TAB_OVERVIEW,	// open the tab overview
	SHOW_BOOKMARKS,	// shows the list of bookmarks
	CLICK,			// click on the nearest clickable element containing the [parameter]
	CHECK,			// check the nearest checkbox
	PLAY,			// play the nearest video 
	PAUSE,			// pause the nearest video 
	MUTE,			// mute the browsers sound
	UNMUTE,			// unmute the browsers sound
	TEXT,			// activate text input ?[parameter]
	REMOVE,			// remove last inputted word
	CLEAR,			// clear the entire text input field
	CLOSE			// close the current view and return to the page view

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
		_ChannelCount(channelCount),
		_SampleRate(sampleRate),
		_MaxSeconds(maxSeconds)
	{
	}

	//! Getter.
	int getChannelCount() const { return _ChannelCount; };
	int getSampleRate() const { return _SampleRate; };
	int getSampleCount() const { return _Index; }

	//! Add sample. Increments index and returns whether successful.
	bool addSample(short sample);
	std::unique_ptr<std::vector<short> > moveBuffer();


private:
	//! Members.
	std::deque<short> _buffer;
	const int _maximumSize;
	bool _maximumSizeReached = false;
	mutable std::mutex _bufferGuard;
	std::shared_ptr<std::vector<short> > _spBuffer;
	unsigned int _ChannelCount;
	unsigned int _SampleRate;
	unsigned int _MaxSeconds;
	unsigned int _Index = 0;
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

	// Update voice input
	VoiceAction Update(float tpf);

	// Start recording and transcribing process.
	void Activate();

	// Stop recording and transcribing process.
	void Deactivate();

	// Returns whether the recording and transcribing process is currently running.
	bool IsActive() { return _active; }

private:

	// Returns whether the plugin itself and all the respective functions are loaded.
	bool IsPluginLoaded();

// [RECORDING]

	static const int AUDIO_INPUT_SAMPLE_RATE = 16000;
	static const int AUDIO_INPUT_CHANNEL_COUNT = 1;
	static const unsigned int AUDIO_INPUT_MAX_INPUT_SECONDS = 3;
	
	bool _portAudioInitialized = false;
	std::shared_ptr<ContinuousAudioRecord> _spAudioInput;

// [STREAMING]

	// language to be transcribed
	char* _language = "en-US";

	// _sample_rate of the audio
	int _sample_rate = 16000;

	// time to query audio in ms
	int _queryTime = 1000;


	// state showing if the transcribing is activate (recording and streaming to google)
	bool _active = false;

	// Thread variable, set in main thread, read in subthreads
	// state showing if RunTranscribing should be stopped
	std::atomic<bool> _stopped = true;

	// thread handling the sending
	std::unique_ptr<std::thread> _tSending = nullptr;
	bool _isSending = false;

	// thread handling the receving
	std::unique_ptr<std::thread> _tReceiving = nullptr;
	bool _isReceiving = false;

	GO_SPEECH_RECOGNITION_INITIALIZE_STREAM	GO_SPEECH_RECOGNITION_InitializeStream;
	GO_SPEECH_RECOGNITION_SEND_AUDIO GO_SPEECH_RECOGNITION_SendAudio;
	GO_SPEECH_RECOGNITION_RECEIVE_TRANSCRIPT GO_SPEECH_RECOGNITION_ReceiveTranscript;
	GO_SPEECH_RECOGNITION_GET_LOG GO_SPEECH_RECOGNITION_GetLog;
	GO_SPEECH_RECOGNITION_CLOSE_STREAM GO_SPEECH_RECOGNITION_CloseStream;
	GO_SPEECH_RECOGNITION_IS_INITIALIZED GO_SPEECH_RECOGNITION_IsInitialized;

// [PROCESSING TRANSCRIPT]

	// last recognized speech as text
	std::queue<std::string> _recognitionResults;

	// protects access to recognition_results
	std::mutex _transcriptGuard;

	// split a string at spaces
	std::vector<std::string> split(std::string text);
};

#endif // VOICEINPUT_H_
