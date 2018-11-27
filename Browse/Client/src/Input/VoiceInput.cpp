//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//		   Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================

#include "VoiceInput.h"
#include "src/Utils/Logger.h"
#include <map>
#include <set>
#include <windows.h>

#include <regex>
#include <algorithm>
#include <iterator>
#include <future>

// Use portaudio library coming with eyeGUI. Bad practice, but eyeGUI would be needed to be linked dynamically otherwise
#include "submodules/eyeGUI/externals/PortAudio/include/portaudio.h"
// #include "submodules/eyeGUI/externals/levenshtein-sse/levenshtein-sse.hpp" -> levenshtein library

PaStream* _pInputStream = nullptr;
HINSTANCE pluginHandle;

std::map<std::string, VoiceCommand> voiceCommandMapping = {

	{ "up",				VoiceCommand::SCROLL_UP },
	{ "app",			VoiceCommand::SCROLL_UP },
	{ "down",			VoiceCommand::SCROLL_DOWN },
	{ "town",			VoiceCommand::SCROLL_DOWN },
	{ "dawn",			VoiceCommand::SCROLL_DOWN },
	{ "dumb",			VoiceCommand::SCROLL_DOWN },
	{ "top",			VoiceCommand::TOP },
	{ "bottom",			VoiceCommand::BOTTOM },
	{ "button",			VoiceCommand::BOTTOM },
	{ "boredom",		VoiceCommand::BOTTOM },
	{ "bookmark",		VoiceCommand::BOOKMARK },
	{ "back",			VoiceCommand::BACK },
	{ "reload",			VoiceCommand::RELOAD },
	{ "forward",		VoiceCommand::FORWARD },
	{ "go to",			VoiceCommand::GO_TO },
	{ "new tab",		VoiceCommand::NEW_TAB },
	{ "new tap",		VoiceCommand::NEW_TAB },
	{ "UTEP",			VoiceCommand::NEW_TAB },
	{ "search",			VoiceCommand::SEARCH },
	{ "zoom",			VoiceCommand::ZOOM },
	{ "tab overview",	VoiceCommand::TAB_OVERVIEW },
	{ "show bookmarks",	VoiceCommand::SHOW_BOOKMARKS },
	{ "click",			VoiceCommand::CLICK },
	{ "clique",			VoiceCommand::CLICK },
	{ "clip",			VoiceCommand::CLICK },
	{ "check",			VoiceCommand::CHECK },
	{ "chuck",			VoiceCommand::CHECK },
	{ "play",			VoiceCommand::PLAY },
	{ "pause",			VoiceCommand::PAUSE },
	{ "mute",			VoiceCommand::MUTE },
	{ "unmute",			VoiceCommand::UNMUTE },
	{ "text",			VoiceCommand::TEXT },
	{ "type",			VoiceCommand::TEXT },
	{ "remove",			VoiceCommand::REMOVE },
	{ "clear",			VoiceCommand::CLEAR },
	{ "close",			VoiceCommand::CLOSE },

};

std::set<std::string> voiceCommandKeys = {

	"up",
	"down",
	"top",
	"bottom",
	"boredom",
	"bookmark",
	"back",
	"reload",
	"forward",
	"go to",
	"new tab",
	"search",
	"zoom",
	"tab overview",
	"show bookmarks",
	"click",
	"check",
	"play",
	"pause",
	"mute",
	"unmute",
	"text",
	"type",
	"remove",
	"clear",
	"close"

};

// Constructor - initializes Portaudio, loads go-speech-recognition.dll and it's functions
VoiceInput::VoiceInput() {

	PaError err;
	err = Pa_Initialize();
	if (err != paNoError)
	{
		LogError("Could not initialize PortAudio.");
		// TODO exit somehow and avoid any further use of this object during runtime
	}
	else {
		_portAudioInitialized = true;
		LogInfo("PortAudio version: " + std::to_string(Pa_GetVersion()));
	}

	// [INITIALIZING] Load Plugins
	if (_portAudioInitialized) {
		// Try to load plugin
		pluginHandle = LoadLibraryA("go-speech-recognition.dll");


		// Check plugin
		if (!pluginHandle)
		{
			LogInfo("VoiceInput: Failed to load go-speech-recognition plugin.");
			return;
		}

		LogInfo("VoiceInput: Loaded successfully go-speech-recognition.dll.");

		GO_SPEECH_RECOGNITION_InitializeStream = reinterpret_cast<GO_SPEECH_RECOGNITION_INITIALIZE_STREAM>(GetProcAddress(pluginHandle, "InitializeStream"));
		if (!GO_SPEECH_RECOGNITION_InitializeStream)
		{
			LogError("VoiceInput: Failed to load function GO_SPEECH_RECOGNITION_InitializeStream.");
			return;
		}

		GO_SPEECH_RECOGNITION_SendAudio = reinterpret_cast<GO_SPEECH_RECOGNITION_SEND_AUDIO>(GetProcAddress(pluginHandle, "SendAudio"));
		if (!GO_SPEECH_RECOGNITION_SendAudio)
		{
			LogError("VoiceInput: Failed to load function GO_SPEECH_RECOGNITION_SendAudio.");
			return;
		}

		GO_SPEECH_RECOGNITION_ReceiveTranscript = reinterpret_cast<GO_SPEECH_RECOGNITION_RECEIVE_TRANSCRIPT>(GetProcAddress(pluginHandle, "ReceiveTranscript"));
		if (!GO_SPEECH_RECOGNITION_ReceiveTranscript)
		{
			LogError("VoiceInput: Failed to load function GO_SPEECH_RECOGNITION_ReceiveTranscript.");
			return;
		}

		GO_SPEECH_RECOGNITION_GetLog = reinterpret_cast<GO_SPEECH_RECOGNITION_GET_LOG>(GetProcAddress(pluginHandle, "GetLog"));
		if (!GO_SPEECH_RECOGNITION_GetLog)
		{
			LogError("VoiceInput: Failed to load function GO_SPEECH_RECOGNITION_GetLog.");
			return;
		}

		GO_SPEECH_RECOGNITION_CloseStream = reinterpret_cast<GO_SPEECH_RECOGNITION_CLOSE_STREAM>(GetProcAddress(pluginHandle, "CloseStream"));
		if (!GO_SPEECH_RECOGNITION_CloseStream)
		{
			LogError("VoiceInput: Failed to load function GO_SPEECH_RECOGNITION_CloseStream.");
			return;
		}

		GO_SPEECH_RECOGNITION_IsInitialized = reinterpret_cast<GO_SPEECH_RECOGNITION_IS_INITIALIZED>(GetProcAddress(pluginHandle, "IsInitialized"));
		if (!GO_SPEECH_RECOGNITION_IsInitialized)
		{
			LogError("VoiceInput: Failed to load function GO_SPEECH_RECOGNITION_IsInitialized.");
			return;
		}

		LogInfo("VoiceInput: Loaded successfully all functions from go-speech-recognition.dll.");
	}
}

// Destructor
VoiceInput::~VoiceInput() {
	Deactivate();
	FreeLibrary(pluginHandle);
}
/*
	TRANSCRIPT TO ACTION
*/
// Returns the last transcribed audio as a VoiceAction object
VoiceAction VoiceInput::Update(float tpf) {

	// Initialize voiceResult (When there's no (matching) transcript from queue it'll be returned as is)
	VoiceAction voiceResult = VoiceAction(VoiceCommand::NO_ACTION, "");
	std::string transcript;

	// Take first transcript from queue
	_transcriptGuard.lock();
	if (!_recognitionResults.empty()) {
		transcript = _recognitionResults.front();
		_recognitionResults.pop();
	}
	_transcriptGuard.unlock();


	if (!transcript.empty()) {

		std::string voiceCommand = "";
		int voiceParameter = 0;
		std::vector<std::string> keySplitList;
		std::vector<std::string> tranSplitList = split(transcript);
		int tranSplitListLen = tranSplitList.size();

		for (std::string key : voiceCommandKeys) {
			//lev distance of key with transciption
			int levCom = 0;
			keySplitList = split(key);
			int keySplitListLen = keySplitList.size();


			for (int i = 0; i < keySplitListLen; i++) {
				for (int j = 0; j < tranSplitListLen; j++) {
					if (i < tranSplitListLen) {
						levCom += uiLevenshteinDistance(tranSplitList[j], keySplitList[i]);
						if (tranSplitList[j] == keySplitList[i] || levCom == 0 || (levCom < 2 && levCom != key.size())) {
							voiceCommand = key;
							voiceParameter = j + 1;
							LogInfo("VoiceInput: voice distance between transcript ( ", tranSplitList[j], " ) and ( ", keySplitList[i], " ) is ", levCom);
						}
					}
				}
			}
		}
		if (!voiceCommand.empty()) {
			voiceResult.command = voiceCommandMapping[voiceCommand];
			voiceResult.parameter = findPrefixAndParameters(voiceResult.command, tranSplitList, voiceParameter);
		}
		LogInfo("voiceCommand: " + voiceCommand + " Parameter: " + voiceResult.parameter);
	}
		
	return voiceResult;
}

/*
	HELPER FOR UPDATE
*/
std::string VoiceInput::findPrefixAndParameters(VoiceCommand command, std::vector<std::string> transcript, int paraIndex) {
	
	std::string parameter = "";

	if (command == VoiceCommand::GO_TO || command == VoiceCommand::NEW_TAB)	{

		if (transcript.size() > 1) {
			if (!transcript[paraIndex].empty()) {
				parameter = transcript[paraIndex];
				size_t found = parameter.find('.');
				if (found == std::string::npos) {
					parameter = transcript[paraIndex] + ".com";
				}
			}
		}
	}

	if (command == VoiceCommand::SEARCH || command == VoiceCommand::CLICK) {

		if (transcript.size() > 1) {
			if (!transcript[paraIndex].empty()) {
				for (int i = paraIndex; i < transcript.size(); i++) { 
					parameter += transcript[i];
				};
			}
		}
	}

	return parameter;
}

// levenshtein distance
size_t VoiceInput::uiLevenshteinDistance(const std::string &s1, const std::string &s2)
{
	const size_t m(s1.size());
	const size_t n(s2.size());

	if (m == 0) return n;
	if (n == 0) return m;

	size_t *costs = new size_t[n + 1];

	for (size_t k = 0; k <= n; k++) costs[k] = k;

	size_t i = 0;
	for (std::string::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1, ++i)
	{
		costs[0] = i + 1;
		size_t corner = i;

		size_t j = 0;
		for (std::string::const_iterator it2 = s2.begin(); it2 != s2.end(); ++it2, ++j)
		{
			size_t upper = costs[j + 1];
			if (*it1 == *it2)
			{
				costs[j + 1] = corner;
			}
			else
			{
				size_t t(upper < corner ? upper : corner);
				costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
			}

			corner = upper;
		}
	}

	size_t result = costs[n];
	delete[] costs;

	return result;
}

// split a string at spaces
std::vector<std::string> VoiceInput::split(std::string text) {
	std::istringstream iss(text);
	std::vector<std::string> splittedList(std::istream_iterator<std::string>{iss},
		std::istream_iterator<std::string>());
	return splittedList;
}

/*
	TRANSCRIBING
*/
void VoiceInput::Activate()
{

	LogInfo("VoiceInput: Started transcribing process.");

	_active = true;
	_stopped = false;
	// [STREAM INITIALIZATION]
	GO_SPEECH_RECOGNITION_BOOL initSuccess = GO_SPEECH_RECOGNITION_InitializeStream(_language, _sample_rate);
	if (initSuccess != GO_SPEECH_RECOGNITION_TRUE) {
		std::string log = GO_SPEECH_RECOGNITION_GetLog();
		LogError("VoiceInput: " + log);
	}

	LogInfo("VoiceInput: Stream was successfully initialized.");

	// [SETUP RECORDING]
	bool recSuccess = StartAudioRecording();
	if (!recSuccess) {
		LogError("VoiceInput: Started audio recording.");
	}

	// [SENDING] loops till it's stopped by calling VoiceInput::StopTranscribing() or VoiceInput::Deactivate()
	_tSending = std::make_unique<std::thread>(([this] {
		_isSending = true;
		ContinuousAudioRecord& pRecord = *_spAudioInput.get();
		
		LogInfo("VoiceInput: Started sending audio.");

		while (!_stopped) {

 			std::this_thread::sleep_for(std::chrono::milliseconds(_queryTime));

			// Retrieve audio
			std::vector<short> audioData;
			pRecord.copyBuffer(&audioData);

			// Check initialization
			GO_SPEECH_RECOGNITION_BOOL initialized = GO_SPEECH_RECOGNITION_IsInitialized();
			if (initialized != GO_SPEECH_RECOGNITION_TRUE) {
				if (_stopped) {
					LogInfo("VoiceInput: Stream is not initialized! (SENDING)");
					_isSending = false;
					return;
				}
				LogError("VoiceInput: Stream is not initialized! (SENDING)");
				_isSending = false;
				return;
			}

			// Send the audio
			GO_SPEECH_RECOGNITION_BOOL sendSuccess = GO_SPEECH_RECOGNITION_SendAudio(audioData.data(), audioData.size());
			if (sendSuccess != GO_SPEECH_RECOGNITION_TRUE) {
				std::string log = GO_SPEECH_RECOGNITION_GetLog();
				LogError("VoiceInput: " + log);
			}
		}
		_isSending = false;
	}));

	// [RECEIVING] loops till it's stopped by calling VoiceInput::StopTranscribing() or VoiceInput::Deactivate()
	_tReceiving = std::make_unique<std::thread>(([this] {
		_isReceiving = true;
		LogInfo("VoiceInput: Started receiving transcripts.");

		while (!_stopped) {

			// Check initialization
			GO_SPEECH_RECOGNITION_BOOL initialized = GO_SPEECH_RECOGNITION_IsInitialized();
			if (initialized != GO_SPEECH_RECOGNITION_TRUE) {
				if (_stopped) {
					LogInfo("VoiceInput: Stream is not initialized! (RECEIVING)");
					_isReceiving = false;
					return;
				}
				LogError("VoiceInput: Stream is not initialized! (RECEIVING)");
				_isReceiving = false;
				return;
			}

			// Receive the transcript
			std::string received = GO_SPEECH_RECOGNITION_ReceiveTranscript();
			if (!received.empty()) {
				LogInfo("VoiceInput: Received: [" + received + "]");
				
				std::lock_guard<std::mutex> lock(_transcriptGuard);
				_recognitionResults.push(received);
			}
		}
		_isReceiving = false;
	}));
}

void VoiceInput::Deactivate() {
		std::thread _tStopping([this] {
		LogInfo("VoiceInput: Stopping audio recording and transcribing process.");

		_stopped = true;
		GO_SPEECH_RECOGNITION_CloseStream();
		EndAudioRecording();

		while (_isSending || _isReceiving) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		};
		_active = false;
		LogInfo("VoiceInput: Stopped audio recording and transcribing process.");
	});
}

/*
	PORTAUDIO/RECORDING
*/

// Static callback for PortAudio input stream
static int audioStreamRecordCallback(
	const void* inputBuffer, // buffer for audio input via microphone
	void* outputBuffer, // buffer for audio output via speakers
	unsigned long framesPerBuffer, // counts of frames (count of samples for all channels)
	const PaStreamCallbackTimeInfo* timeInfo, // not used
	PaStreamCallbackFlags flags, // not used
	void* data) // pointer to audio data
{

	// Cast input buffer
	short* in = (short*)inputBuffer;

	// Prevent unused variable warnings
	(void)timeInfo;
	(void)flags;

	auto pData = reinterpret_cast<ContinuousAudioRecord*>(data);

	for (unsigned int i = 0; i < framesPerBuffer; i++) // go over frames
	{
		for (unsigned int j = 0; j < pData->getChannelCount(); j++) // go over channels
		{
			pData->addSample(*in++);
		}
	}
	return PaStreamCallbackResult::paContinue;
}

bool VoiceInput::StartAudioRecording()
{
	// Check for PortAudio
	if (!_portAudioInitialized)
	{
		// DEBUG
		LogError("PortAudio not initialized!");
		return false;
	}

	// Return false if ongoing recording
	if (_pInputStream != nullptr)
	{
		// DEBUG
		LogInfo("Recording still in progress...");
		return false;
	}

	_spAudioInput = std::shared_ptr<ContinuousAudioRecord>(new ContinuousAudioRecord(
		AUDIO_INPUT_CHANNEL_COUNT,
		AUDIO_INPUT_SAMPLE_RATE,
		AUDIO_INPUT_MAX_INPUT_SECONDS));

	// Set up stream
	PaStreamParameters parameters;
	parameters.device = Pa_GetDefaultInputDevice();
	parameters.channelCount = AUDIO_INPUT_CHANNEL_COUNT;
	parameters.sampleFormat = paInt16;
	parameters.suggestedLatency = Pa_GetDeviceInfo(parameters.device)->defaultLowInputLatency;
	parameters.hostApiSpecificStreamInfo = NULL;

	if (parameters.device == paNoDevice)
	{
		LogError("PortAudio error: Have not found an input audio device");
		return false;
	}

	PaError err; // Variable to fetch errors

	// Open stream
	err = Pa_OpenStream(&_pInputStream, &parameters, NULL, AUDIO_INPUT_SAMPLE_RATE,
		paFramesPerBufferUnspecified, paClipOff, audioStreamRecordCallback, _spAudioInput.get());
	if (err != paNoError)
	{
		LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
		_pInputStream = nullptr;
		return false;
	}

	// Start stream
	err = Pa_StartStream(_pInputStream);
	if (err != paNoError)
	{
		LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
		_pInputStream = nullptr; // possible memory leak?
		return false;
	}

	return true;
}

bool VoiceInput::EndAudioRecording()
{
	// Check for PortAudio
	if (!_portAudioInitialized)
	{
		return false;
	}

	// Variable to fetch errors
	PaError err;

	LogInfo("VoiceInput: Stopping audio recording.");

	// Aborts stream (stop would wait until buffer finished)
	err = Pa_AbortStream(_pInputStream);
	if (err != paNoError)
	{
		LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
		_pInputStream = NULL;
		return false;
	}

	// Close stream
	err = Pa_CloseStream(_pInputStream);
	if (err != paNoError)
	{
		LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
		_pInputStream = NULL;
		return false;
	}

	// Set stream to NULL
	_pInputStream = NULL;


	// Success
	return true;
}

/*
	CLASS ContinuousAudioRecord
*/

/*
Adds the inputted sample to the std::deque<short> _buffer,
keeps _buffer.size() <= _maximumSize
(secured by mutex _bufferGuard)

Parameters:
(short) sample: sample to be added to _buffer

Return:
bool: true if correctly finished
*/
bool ContinuousAudioRecord::addSample(short sample)
{
	std::lock_guard<std::mutex> lock(_bufferGuard);

	if (!_maximumSizeReached)
	{
		_maximumSizeReached = static_cast<int>(_buffer.size()) == _maximumSize;
		_Index = _buffer.size();
	}
	if (_maximumSizeReached)
	{
		_buffer.pop_front();
	}

	_buffer.push_back(sample);
	return true;
}

void ContinuousAudioRecord::copyBuffer(std::vector<short> *audioData)
{
	std::lock_guard<std::mutex> lock(_bufferGuard);

	audioData->resize(_buffer.size());

	std::copy(std::begin(_buffer), std::end(_buffer), std::begin(*audioData));
	_buffer.clear();
}
