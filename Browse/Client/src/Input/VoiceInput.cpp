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
#include <iterator>

// levenshtein library
#include "submodules/eyeGUI/externals/levenshtein-sse/levenshtein-sse.hpp"

// Use portaudio library coming with eyeGUI. Bad practice, but eyeGUI would be needed to be linked dynamically otherwise
#include "submodules/eyeGUI/externals/PortAudio/include/portaudio.h"

PaStream* _pInputStream = nullptr;
HINSTANCE pluginHandle;

// const std::string _triggerKey = "voice";

// maybe set with tuples? with a bool indicating if a parameter is needed (could make VoiceInput::Update() a bit more dynamic)
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


		if (IsPluginLoaded())
		{
			LogInfo("VoiceInput: Loaded successfully all functions from go-speech-recognition.dll.");
		}
		else {
			LogError("VoiceInput: go-speech-recognition.dll is not properly loaded.");
		}
	}
}


// Problem! Deactivate runs in it's own thread
// Destructor
VoiceInput::~VoiceInput() {
	Deactivate();
	if (pluginHandle) {
		FreeLibrary(pluginHandle);
	}
}

bool VoiceInput::IsPluginLoaded() {
	return pluginHandle && GO_SPEECH_RECOGNITION_InitializeStream && GO_SPEECH_RECOGNITION_SendAudio
		&& GO_SPEECH_RECOGNITION_ReceiveTranscript && GO_SPEECH_RECOGNITION_GetLog
		&& GO_SPEECH_RECOGNITION_CloseStream && GO_SPEECH_RECOGNITION_IsInitialized;
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

	// Assignment of voice action begins
	if (!transcript.empty()) {

		std::string voiceCommand = "";
		// Index representing the beginning of the parameter (index of the last word of a key + 1)
		int voiceParameterIndex = 0;
		std::vector<std::string> tranSplitList = split(transcript);
		int tranSplitListLen = tranSplitList.size();

/*
		bool triggered = false;
		int triggerIndex;

		// Check transcript for _triggerKey ("voice")
		for (int i = 0; i < tranSplitListLen; i++ ) {
			int levDistance = levenshteinSSE::levenshtein(tranSplitList[i], _triggerKey);
			if (levDistance < 2) {
				triggered = true;
				triggerIndex = i;
			}
		}

		// Check for voice command after _triggerKey
		if (triggered) {
			for (auto const pair : voiceCommandMapping) {
				std::string key = pair.first;

				// lev distance between key and transcription
				int levDistance = 0;

				// vector representing a key (needed because some keys are longer than 1 word)
				std::vector<std::string> keySplitList = split(key);
				int keySplitListLen = keySplitList.size();

				// iterate over the key
				for (int i = 0; i < keySplitListLen; i++) {
					// don't access indexes > than tranSplitListLen to avoid exceptions
					if (i < tranSplitListLen - 1) {
						// check the Levenshtein Distance between the current word in the key and the current word in the transcript
						levDistance += levenshteinSSE::levenshtein(keySplitList[i], tranSplitList[triggerIndex + 1 + i]);
						// if the transcript is similiar enough (Levenshtein Distance < 2) to the key the command is found
						if (levDistance < 2 && i == keySplitListLen - 1) {
							voiceCommand = key;
							voiceParameterIndex = triggerIndex + 1 + i;
							LogInfo("VoiceInput: voice distance between transcript ( ", tranSplitList[triggerIndex + 1 + i], " ) and ( ", keySplitList[i], " ) is ", levDistance);
						}
					}
				}
			}
		}
*/
	
		int shortestLevDistance = INT32_MAX;

		// iterate over all keys
		for (auto const pair : voiceCommandMapping) {
			std::string key = pair.first;

			// lev distance of key with transciption
			int levDistance = 0;
			std::vector<std::string> keySplitList = split(key);
			int keySplitListLen = keySplitList.size();

			// iterate over the key
			for (int i = 0; i < tranSplitListLen; i++) {

				// check the Levenshtein Distance between the first word in the key and the current word in the transcript
				levDistance = levenshteinSSE::levenshtein(tranSplitList[i], keySplitList[0]);
				
				// if the transcript is similiar enough (Levenshtein Distance < 2) to the key the command could be (partially) found
				if (levDistance < 2 && levDistance < shortestLevDistance) {

					// check if the following words are matching if the key is longer than 1 word
					if (keySplitListLen > 1) {

						// iterate over the rest of the keySplitList
						for (int j = 1; j < keySplitListLen; j++) {

							// levDistance has to be < 2 for the whole key 
							levDistance += levenshteinSSE::levenshtein(tranSplitList[i + j], keySplitList[j]);

							// we found the currently best matching key
							if (levDistance < 2 && levDistance < shortestLevDistance && j == keySplitListLen - 1) {
								voiceCommand = key;
								voiceParameterIndex = i + j + 1;
								LogInfo("VoiceInput: voice distance between transcript ( ", tranSplitList[i + j], " ) and ( ", keySplitList[j], " ) is ", levDistance);
								shortestLevDistance = levDistance;
							}
						}
					}

					// the key is one word and the currently best matching one
					else {
						voiceCommand = key;
						voiceParameterIndex = i + 1;
						LogInfo("VoiceInput: voice distance between transcript ( ", tranSplitList[i], " ) and ( ", keySplitList[0], " ) is ", levDistance);
						shortestLevDistance = levDistance;
					}
				}
			}
		}

		// assign the found command to the voiceResult
		if (!voiceCommand.empty()) {
			voiceResult.command = voiceCommandMapping[voiceCommand];

			// assign parameter if needed
			if (VoiceCommand::NEW_TAB == voiceResult.command
				|| VoiceCommand::GO_TO == voiceResult.command
				|| VoiceCommand::SEARCH == voiceResult.command
				|| VoiceCommand::CLICK == voiceResult.command
				|| VoiceCommand::TEXT == voiceResult.command) {
				
				// add the transcripted words following the command to the voiceResult.parameter
				for (int i = voiceParameterIndex; i < tranSplitListLen; i++) {			
					if (i == voiceParameterIndex) {
						voiceResult.parameter += tranSplitList[i];
					}
					else {
						voiceResult.parameter += " " + tranSplitList[i];
					}			
				}
				
			}
		}
		LogInfo("voiceCommand: " + voiceCommand + " Parameter: " + voiceResult.parameter);
	}
	

	return voiceResult;
}

/*
HELPER FOR UPDATE
*/

// split a string at spaces
std::vector<std::string> VoiceInput::split(std::string text) {
	std::istringstream iss(text);
	std::vector<std::string> splittedList(std::istream_iterator<std::string>{iss},
		std::istream_iterator<std::string>());
	return splittedList;
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

/*
TRANSCRIBING
*/
void VoiceInput::Activate() {

	if (IsPluginLoaded()) {
		LogInfo("VoiceInput: Started transcribing process.");

		_active = true;
		_stopped = false;

		// [STREAM INITIALIZATION]
		GO_SPEECH_RECOGNITION_BOOL initSuccess = GO_SPEECH_RECOGNITION_InitializeStream(_language, _sample_rate);
		if (initSuccess != GO_SPEECH_RECOGNITION_TRUE || !IsPluginLoaded()) {
			std::string log = GO_SPEECH_RECOGNITION_GetLog();
			LogError("VoiceInput: " + log + " (INITIALIZING)");
		}

		LogInfo("VoiceInput: Stream was successfully initialized.");

		// [SETUP RECORDING]
		// Check for PortAudio
		if (!_portAudioInitialized)
		{
			// DEBUG
			LogError("PortAudio not initialized!");
			return;
		}

		// Return false if ongoing recording
		if (_pInputStream != nullptr)
		{
			// DEBUG
			LogInfo("Recording still in progress...");
			return;
		}

		_spAudioInput = std::shared_ptr<ContinuousAudioRecord>(new ContinuousAudioRecord(
			AUDIO_INPUT_CHANNEL_COUNT,
			AUDIO_INPUT_SAMPLE_RATE,
			AUDIO_INPUT_MAX_INPUT_SECONDS));

		// Set up stream
		PaStreamParameters parameters;
		parameters.device = Pa_GetDefaultInputDevice();

		// Get and log name of used device 
		std::string deviceName = Pa_GetDeviceInfo(parameters.device)->name;
		LogInfo("PortAudio: Chosen input audio device: " + deviceName);

		parameters.channelCount = AUDIO_INPUT_CHANNEL_COUNT;
		parameters.sampleFormat = paInt16;
		parameters.suggestedLatency = Pa_GetDeviceInfo(parameters.device)->defaultLowInputLatency;
		parameters.hostApiSpecificStreamInfo = NULL;

		if (parameters.device == paNoDevice)
		{
			LogError("PortAudio error: Have not found an input audio device");
			return;
		}

		PaError err; // Variable to fetch errors

					 // Open stream
		err = Pa_OpenStream(&_pInputStream, &parameters, NULL, AUDIO_INPUT_SAMPLE_RATE,
			paFramesPerBufferUnspecified, paClipOff, audioStreamRecordCallback, _spAudioInput.get());
		if (err != paNoError)
		{
			LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
			_pInputStream = nullptr;
			return;
		}

		// Start stream
		err = Pa_StartStream(_pInputStream);
		if (err != paNoError)
		{
			LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
			_pInputStream = nullptr; // possible memory leak?
			return;
		}


		// [SENDING] loops till it's stopped by calling VoiceInput::StopTranscribing() or VoiceInput::Deactivate()
		_tSending = std::make_unique<std::thread>([this] {
			_isSending = true;
			ContinuousAudioRecord& pRecord = *_spAudioInput.get();

			LogInfo("VoiceInput: Started sending audio.");

			while (!_stopped) {

				std::this_thread::sleep_for(std::chrono::milliseconds(_queryTime));

				// Retrieve audio
				auto upAudioData = pRecord.moveBuffer();

				// Check initialization
				GO_SPEECH_RECOGNITION_BOOL initialized = GO_SPEECH_RECOGNITION_IsInitialized();
				if (initialized != GO_SPEECH_RECOGNITION_TRUE || !IsPluginLoaded()) {
					if (_stopped) {
						LogInfo("VoiceInput: Stream is not initialized! (SENDING)");
					}
					else {
						LogError("VoiceInput: Stream is not initialized! (SENDING)");
					}
					_isSending = false;
					return;
				}

				// Send the audio
				GO_SPEECH_RECOGNITION_BOOL sendSuccess = GO_SPEECH_RECOGNITION_SendAudio(upAudioData->data(), upAudioData->size());
				if (sendSuccess != GO_SPEECH_RECOGNITION_TRUE || !IsPluginLoaded()) {
					std::string log = GO_SPEECH_RECOGNITION_GetLog();
					LogError("VoiceInput: " + log + " (SENDING)");
					_stopped = true;
					_isSending = false;
					return;
				}
			}
			_isSending = false;
		});

		// [RECEIVING] loops till it's stopped by calling VoiceInput::StopTranscribing() or VoiceInput::Deactivate()
		_tReceiving = std::make_unique<std::thread>([this] {
			_isReceiving = true;
			LogInfo("VoiceInput: Started receiving transcripts.");

			while (!_stopped) {

				std::string receivedString;
				char* received;

				// Check initialization
				GO_SPEECH_RECOGNITION_BOOL initialized = GO_SPEECH_RECOGNITION_IsInitialized();
				if (initialized != GO_SPEECH_RECOGNITION_TRUE || !IsPluginLoaded()) {
					if (_stopped) {
						LogInfo("VoiceInput: Stream is not initialized! (RECEIVING)");
					}
					else {
						LogError("VoiceInput: Stream is not initialized! (RECEIVING)");
					}
					_isReceiving = false;
					return;
				}

				// Receive the transcript
				GO_SPEECH_RECOGNITION_BOOL receiveSuccess = GO_SPEECH_RECOGNITION_ReceiveTranscript(&received);
				if (receiveSuccess != GO_SPEECH_RECOGNITION_TRUE) {
					std::string log = GO_SPEECH_RECOGNITION_GetLog();
					LogError("VoiceInput: " + log + " (RECEIVING)");
					_stopped = true;
					_isReceiving = false;
					return;
				}

				receivedString = received;
				if (!receivedString.empty()) {
					LogInfo("VoiceInput: Received: [" + receivedString + "]");

					std::lock_guard<std::mutex> lock(_transcriptGuard);
					_recognitionResults.push(receivedString);
				}
			}
			_isReceiving = false;
		});
	}
}

void VoiceInput::Deactivate() {
	std::thread _tStopping([this] {
		LogInfo("VoiceInput: Stopping audio recording and transcribing process.");

		_stopped = true;
		GO_SPEECH_RECOGNITION_CloseStream();

		// STOP AUDIO RECORDING
		// Check for PortAudio
		if (_portAudioInitialized)
		{
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
		}

		while (_isSending || _isReceiving) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		};
		_active = false;
		LogInfo("VoiceInput: Stopped audio recording and transcribing process.");
	});
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

std::unique_ptr<std::vector<short> > ContinuousAudioRecord::moveBuffer()
{
	std::lock_guard<std::mutex> lock(_bufferGuard);

	std::vector<short> audioData(_buffer.size());

	std::copy(std::begin(_buffer), std::end(_buffer), std::begin(audioData));

	_buffer.clear();

	return std::make_unique<std::vector<short> >(audioData);
}
