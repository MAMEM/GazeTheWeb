//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Stub for voice input handling.

// TODO @ Christopher: Change this interface as desired!

#ifndef VOICEINPUT_H_
#define VOICEINPUT_H_

#include <windows.h>
#include <thread>
#include <memory>
#include <vector>
#include <deque>
#include <mutex>

#include "go-speech-recognition.h"

static const int AUDIO_INPUT_SAMPLE_RATE = 16000;
static const int AUDIO_INPUT_CHANNEL_COUNT = 1;
static const unsigned int AUDIO_INPUT_MAX_INPUT_SECONDS = 3;

enum class VoiceAction
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
	NEW_TAB,		// create a new tab
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
	TEXT,			// activate text input
	REMOVE,			// remove last inputted word
	CLEAR,			// clear the entire text input field
	CLOSE			// close the current view and return to the page view
};					

// VoiceCommand: Action(Parameter)
struct VoiceCommand {

	VoiceAction action;
	std::string parameter;

	VoiceCommand(VoiceAction action, std::string parameter) {
		this->action = action;
		this->parameter = parameter;
	}
};

//! Class for holding audio records.
class AudioRecord
{
public:
	//! Constructor.
	AudioRecord(unsigned int channelCount, unsigned int sampleRate, unsigned int maxSeconds)
		: mChannelCount(channelCount)
		, mSampleRate(sampleRate)
		, mMaxSeconds(maxSeconds) {};

	//! Add sample.
	virtual bool addSample(short sample) = 0;

	//! Getter.
	int getChannelCount() const { return mChannelCount; };
	int getSampleRate() const { return mSampleRate; };
	int getSampleCount() const { return mIndex; }

protected:
	//! Members.
	std::shared_ptr<std::vector<short>> mspBuffer;
	unsigned int mChannelCount;
	unsigned int mSampleRate;
	unsigned int mMaxSeconds;
	unsigned int mIndex = 0;
};


//! Class for holding continuous audio records.
class ContinuousAudioRecord : public AudioRecord
{
public:
	//! Constructor.
	ContinuousAudioRecord(
		unsigned int channelCount, unsigned int sampleRate, unsigned int maxSeconds)
		: maximum_size(channelCount * sampleRate * maxSeconds)
		, AudioRecord(channelCount, sampleRate, maxSeconds)
	{
	}

	//! Add sample. Increments index and returns whether successful.
	bool addSample(short sample);

	//! Getter.
	std::vector<short> getBuffer();


private:
	//! Members.
	std::deque<short> buffer;
	const int maximum_size;
	bool maximum_size_reached = false;
	mutable std::mutex buffer_guard;
};


class VoiceInput
{
public:

	// Constructor
	VoiceInput();

	// Destructor
	virtual ~VoiceInput();

	// Update voice input
	VoiceAction Update(float tpf);

	void addTranscript(std::string transcript);

	std::string getLastTranscript();



	bool portAudioInitialized = false;
	std::shared_ptr<ContinuousAudioRecord> pAudioInput;

	bool startAudioRecording();
	bool endAudioRecording();
	std::shared_ptr<ContinuousAudioRecord> retrieveAudioRecord();

	void setAudioRecordingTime(int ms) { audio_recording_time_ms = ms; }

private:

	// Last recognized speech as text
	std::vector<std::string> recognition_results;
	int audio_recording_time_ms = AUDIO_INPUT_MAX_INPUT_SECONDS;
	std::mutex mutexTranscript; // protects access to recognition_results 
};

class Transcriber : public VoiceInput
{
public:
	// Constructor
	Transcriber();
	
	// Destructor
	~Transcriber();

	void run();
	void stop();

	// Change language of transcribed audio
	void setLanguage(char* lang) { language = lang; }

private:

	HINSTANCE plugin_handle;

	INITIALIZE_STREAM InitializeStream;
	SEND_AUDIO SendAudio;
	RECEIVE_TRANSCRIPT ReceiveTranscript;
	CLOSE_STREAM CloseStream;
	GET_LOG GetLog;
	IS_INITIALIZED IsInitialized;


	char* language = "en-US";
	int sample_rate = 16000;
	int queryTime = 250;
	bool isRunning = false;
	bool stopped = true;

	void initializeStream();
	void sendAudio(ContinuousAudioRecord & record);
	void receiveTranscript();

	std::unique_ptr<std::thread> sending_thread;

//	std::vector<std::string>* recognition_results;

};



#endif // VOICEINPUT_H_