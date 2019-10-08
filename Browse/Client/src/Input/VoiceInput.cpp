//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//		   Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================

#include "VoiceInput.h"
#include "src/Utils/Logger.h"
#include "src/Utils/helper.h"
#include <map>
#include <windows.h>
#include <iterator>
#include "src/Setup.h"
#include "src/Singletons/VoiceMonitorHandler.h"

// Use portaudio library coming with eyeGUI. Bad practice, but eyeGUI would be needed to be linked dynamically otherwise
#include "submodules/eyeGUI/externals/PortAudio/include/portaudio.h"


/*
Thread variable:
Read in: main, _tStopping
Manipulated in: main, _tStopping
*/
PaStream* _pInputStream = nullptr;
HINSTANCE pluginHandle;

std::vector<CommandStruct> commandStructList = {

	CommandStruct(VoiceCommand::SCROLL_UP,		std::vector<std::string> {"scroll up", "up", "app", "call down" },									false),
	CommandStruct(VoiceCommand::SCROLL_DOWN,	std::vector<std::string> {"scroll down", "down", "town", "dawn", "dumb", "call up", "Trov down"},			false),
	CommandStruct(VoiceCommand::TOP,			std::vector<std::string> {"top", "talk"},									false),
	CommandStruct(VoiceCommand::BOTTOM,			std::vector<std::string> {"bottom", "button", "boredom", "autumn"},					false),
	CommandStruct(VoiceCommand::BOOKMARK,		std::vector<std::string> {"bookmark"},											false),
	CommandStruct(VoiceCommand::BACK,			std::vector<std::string> {"back"},												false),
	CommandStruct(VoiceCommand::REFRESH,		std::vector<std::string> {"reload", "refresh"},									false),
	CommandStruct(VoiceCommand::FORWARD,		std::vector<std::string> {"forward", "for what", "for want"},							false),
	CommandStruct(VoiceCommand::GO_TO,			std::vector<std::string> {"go to", "visit"},								true),
	CommandStruct(VoiceCommand::NEW_TAB,		std::vector<std::string> {"new tab", "new tap", "UTEP"},					true),
	CommandStruct(VoiceCommand::SEARCH,			std::vector<std::string> {"search"},											true),
	CommandStruct(VoiceCommand::ZOOM,			std::vector<std::string> {"zoom"},												false),
	CommandStruct(VoiceCommand::TAB_OVERVIEW,	std::vector<std::string> {"tab overview", "tap overview"},					false),
	CommandStruct(VoiceCommand::SHOW_BOOKMARKS, std::vector<std::string> {"show bookmarks"},									false),
	CommandStruct(VoiceCommand::CLICK,			std::vector<std::string> {"click", "lick", "blick", "clique", "clip", "Kik", "Nick", "dick", "big"},				true),
	CommandStruct(VoiceCommand::CHECK,			std::vector<std::string> {"check", "chuck", "checkbox" "checkbook's"},		false),
	CommandStruct(VoiceCommand::VIDEO_INPUT,	std::vector<std::string> {"video", "video input"},							false),
	CommandStruct(VoiceCommand::INCREASE,		std::vector<std::string> {"increase", "increase volume", "increase sound"},	false),
	CommandStruct(VoiceCommand::DECREASE,		std::vector<std::string> {"decrease", "decrease volume", "decrease sound"},	false),
	CommandStruct(VoiceCommand::PLAY,			std::vector<std::string> {"play"},												false),
	CommandStruct(VoiceCommand::PAUSE,			std::vector<std::string> {"pause"},												false),
	CommandStruct(VoiceCommand::STOP,			std::vector<std::string> {"stop"},												false),
	CommandStruct(VoiceCommand::MUTE,			std::vector<std::string> {"mute"},												false),
	CommandStruct(VoiceCommand::UNMUTE,			std::vector<std::string> {"unmute"},											false),
	CommandStruct(VoiceCommand::TEXT,			std::vector<std::string> {"text", "type"},										true),
	CommandStruct(VoiceCommand::REMOVE,			std::vector<std::string> {"remove"},											false),
	CommandStruct(VoiceCommand::CLEAR,			std::vector<std::string> {"clear", "Thalia", "Clea"},						false),
	CommandStruct(VoiceCommand::SUBMIT,			std::vector<std::string> {"submit"},											false),
	CommandStruct(VoiceCommand::CLOSE,			std::vector<std::string> {"close"},												false),
	CommandStruct(VoiceCommand::QUIT,			std::vector<std::string> {"quit"},												false),
	CommandStruct(VoiceCommand::PARAMETER_ONLY,	std::vector<std::string> {},													true),

};

// Constructor - initializes Portaudio, loads go-speech-recognition.dll and it's functions
VoiceInput::VoiceInput(bool allowRestart, bool &finished) {
	
	_allowRestart = allowRestart;
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
			LogError("VoiceInput: Failed to load go-speech-recognition plugin.");
			finished = false;
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
	_startTime = std::chrono::steady_clock::now();
	
	finished = true;
}


// Destructor
VoiceInput::~VoiceInput() {
	if (IsPluginLoaded())
		Deactivate();
	if (pluginHandle)
		FreeLibrary(pluginHandle);
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
std::shared_ptr<VoiceAction> VoiceInput::Update(float tpf, bool keyboardActive) {

	// Keep VoiceMonitorHandlerOnTop (maybe there's a better place to do this?)
	DWORD dwExStyle = GetWindowLong(VoiceMonitorHandler::instance().getWindow(), GWL_EXSTYLE);
	if ((dwExStyle & WS_EX_TOPMOST) == 0)
		SetWindowPos(VoiceMonitorHandler::instance().getWindow(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	

	auto sentSeconds = std::chrono::steady_clock::now() - _startTime;
	std::wstring sentSecondsW = _converter.from_bytes(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(sentSeconds).count()));
	VoiceMonitorHandler::instance().setNewText(PrintCategory::SENTSECONDS, sentSecondsW);

	// Initialize voiceResult (When there's no (matching) transcript from queue it'll be returned as is)
	VoiceAction voiceResult = VoiceAction(VoiceCommand::NO_ACTION, "");
	std::string transcriptCandidatesStr;

	// Better place?
	// check if the time is up
	if (setup::PERIODICAL_VOICE_RESTART && std::chrono::steady_clock::now() - _activationTime > _runTimeLimit)
		this->Reactivate();

	// Handle the keyboard activation/deactivation
	if (keyboardActive && (_voiceMode != VoiceMode::FREE) && (_voiceInputState != VoiceInputState::Changing))
		SetVoiceMode(VoiceMode::FREE);

	else if (!keyboardActive && (_voiceMode != VoiceMode::COMMAND) && (_voiceInputState != VoiceInputState::Changing))
		SetVoiceMode(VoiceMode::COMMAND);


	// Take first transcript from queue
	_transcriptGuard.lock();
	if (!_recognitionResults.empty()) {
		transcriptCandidatesStr = _recognitionResults.front();
		_recognitionResults.pop();
	}
	_transcriptGuard.unlock();


	/*
	TODO:
	Maybe change the following process if the requirements are more specific.
	*/

	// Assignment of voice action begins
	if (!transcriptCandidatesStr.empty()) {

		LogInfo("transcriptCandidatesStr: " + transcriptCandidatesStr);

		std::vector<std::string> transcriptCandidatesList = SplitBySeparator(transcriptCandidatesStr, ';');

		// Because the first candidate has the highest confidence and the process will always overwrite the current best result
		// when found a new one, therefore we iterate back to front through the list
		for (int i = transcriptCandidatesList.size() - 1; i >= 0; i--) {
			LogInfo("transcript: " + transcriptCandidatesList[i]);

			// Index representing the beginning of the parameter (index of the last word of a key + 1)
			int voiceParameterIndex = 0;

			// For comparing purpose split the transcript
			std::vector<std::string> splittedTranscript = SplitBySeparator(transcriptCandidatesList[i], ' ');
			int splittedTranscriptLen = splittedTranscript.size();

			// The current shortest shortest Levenshtein Distance in the transcript
			int shortestStrDistance = INT32_MAX;

			// The best matching CommandStruct
			CommandStruct bestCommandStruct(VoiceCommand::NO_ACTION, std::vector<std::string> {""}, false);

			// iterate over all possible keys
			for (CommandStruct currentCommandStruct : commandStructList) {
				// iterate over all phonetic variants
				for (std::string phoneticVariant : currentCommandStruct.phoneticVariants) {

					// distance between key and transcription
					int strDistance = 0;
					std::vector<std::string> splittedPhoneticVariant = SplitBySeparator(phoneticVariant, ' ');
					int splittedPhoneticVariantLen = splittedPhoneticVariant.size();

					// iterate over the phonetic variant
					for (int i = 0; i < splittedTranscriptLen; i++) {

						// check the Levenshtein Distance between the first word in the phonetic variant and the current word in the transcript
						strDistance = StringDistance(splittedTranscript[i], splittedPhoneticVariant[0], true);

						// if the transcript is similiar enough (Levenshtein Distance < 2) to the key the command could be (partially) found
						if (strDistance < 2 && strDistance < shortestStrDistance) {

							// check if the following words are matching if the phonetic variant is longer than 1 word
							if (splittedPhoneticVariantLen > 1) {

								// iterate over the rest of the splittedPhoneticVariant
								for (int j = 1; j < splittedPhoneticVariantLen && i + j < splittedTranscriptLen; j++) {

									// strDistance has to be < 2 for the whole key 
									strDistance += StringDistance(splittedTranscript[i + j], splittedPhoneticVariant[j], true);

									// we found the currently best matching key
									if (strDistance < 2 && strDistance < shortestStrDistance && j == splittedPhoneticVariantLen - 1) {
										bestCommandStruct = currentCommandStruct;
										voiceParameterIndex = i + j + 1;
										LogInfo("VoiceInput: voice distance between transcript ( ", splittedTranscript[i + j], " ) and ( ", splittedPhoneticVariant[j], " ) is ", strDistance);
										shortestStrDistance = strDistance;
									}
								}
							}

							// the key is one word and the currently best matching one
							else {
								bestCommandStruct = currentCommandStruct;
								voiceParameterIndex = i + 1;
								LogInfo("VoiceInput: voice distance between transcript ( ", splittedTranscript[i], " ) and ( ", splittedPhoneticVariant[0], " ) is ", strDistance);
								shortestStrDistance = strDistance;
							}
						}
					}
				}
			}

			// assign the found command to the voiceResult
			if (bestCommandStruct.command != VoiceCommand::NO_ACTION) {
				voiceResult.command = bestCommandStruct.command;

				// assign parameter if needed
				if (bestCommandStruct.takesParameter) {
					voiceResult.parameter = "";
					// add the transcripted words following the command to the voiceResult.parameter
					for (int i = voiceParameterIndex; i < splittedTranscriptLen; i++) {
						if (i == voiceParameterIndex) {
							voiceResult.parameter += splittedTranscript[i];
						}
						else {
							voiceResult.parameter += " " + splittedTranscript[i];
						}
					}

				}
				LogInfo("voiceCommand: " + bestCommandStruct.phoneticVariants[0] + (!voiceResult.parameter.empty() ? " Parameter: " + voiceResult.parameter : ""));

				std::wstring action = _converter.from_bytes(bestCommandStruct.phoneticVariants[0]);
				VoiceMonitorHandler::instance().setNewText(PrintCategory::CURRENTACTION, action);
			}
			else if (_voiceMode == VoiceMode::FREE) {
				voiceResult.command = VoiceCommand::PARAMETER_ONLY;
				voiceResult.parameter = transcriptCandidatesList[0];
				VoiceMonitorHandler::instance().setNewText(PrintCategory::CURRENTACTION, L"text");
			}
		}
	}


	return std::make_shared<VoiceAction>(voiceResult);

}

void VoiceInput::SetVoiceMode(VoiceMode voiceMode) {
	_voiceInputState = VoiceInputState::Changing;
	VoiceMonitorHandler::instance().setNewText(PrintCategory::CONNECTIONGOOGLE, L"off");

	// voice mode hasn't changed - nothing to do
	if (voiceMode == _voiceMode)
		return;

	_voiceMode = voiceMode;

	if (_voiceMode == VoiceMode::COMMAND) {
		_model = "command_and_search";
	}
	else if (_voiceMode == VoiceMode::FREE) {
		_model = "default";
	}

	this->Reactivate();
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
		for (unsigned int j = 0; j < pData->GetChannelCount(); j++) // go over channels
		{
			pData->AddSample(*in++);
		}
	}
	return PaStreamCallbackResult::paContinue;
}

/*
TRANSCRIBING
*/
void VoiceInput::Activate() {
	_voiceInputState = VoiceInputState::Active;
	_activationTime = std::chrono::steady_clock::now();
	VoiceMonitorHandler::instance().setNewText(PrintCategory::CONNECTIONGOOGLE, L"on");

	if (IsPluginLoaded()) {
		LogInfo("VoiceInput: Started transcribing process.");

		_stopping = false;

		// [STREAM INITIALIZATION]
		GO_SPEECH_RECOGNITION_BOOL initSuccess = GO_SPEECH_RECOGNITION_InitializeStream(_language, _sampleRate, _model, _maxAlternatives, _interimResults);
		if (initSuccess != GO_SPEECH_RECOGNITION_TRUE || !IsPluginLoaded()) {
			std::string log = GO_SPEECH_RECOGNITION_GetLog();
			LogError("VoiceInput: " + log + " (INITIALIZING)");
		}

		LogInfo("VoiceInput: Stream was successfully initialized: Transcription Language: " + std::string(_language)
			+ " | Sample Rate: " + std::to_string(_sampleRate)
			+ " | Transcription Model: " + std::string(_model)
			+ " | Max Alternatives: " + std::to_string(_maxAlternatives)
			+ " | Interim Results: " + (_interimResults == GO_SPEECH_RECOGNITION_TRUE ? "true" : "false")
		);

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

		if (parameters.device == paNoDevice)
		{
			LogError("PortAudio error: Have not found an input audio device");
			return;
		}
		// Get and log name of used device 
		std::string deviceName = Pa_GetDeviceInfo(parameters.device)->name;
		LogInfo("PortAudio: Chosen input audio device: " + deviceName);
		
		VoiceMonitorHandler::instance().setNewText(PrintCategory::CURRENTMICROPHONE, _converter.from_bytes(deviceName));

		parameters.channelCount = AUDIO_INPUT_CHANNEL_COUNT;
		parameters.sampleFormat = paInt16;
		parameters.suggestedLatency = Pa_GetDeviceInfo(parameters.device)->defaultLowInputLatency;
		parameters.hostApiSpecificStreamInfo = NULL;



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

		_voiceInputState = VoiceInputState::Active;

		_tSending = std::make_unique<std::thread>([this] {
			_isSending = true;
			ContinuousAudioRecord& pRecord = *_spAudioInput.get();

			LogInfo("VoiceInput: Started sending audio.");
			VoiceMonitorHandler::instance().setNewText(PrintCategory::CONNECTIONGOOGLE, L"on");

			while (!_stopping) {

				std::this_thread::sleep_for(std::chrono::milliseconds(_queryTime));

				// Retrieve audio
				auto upAudioData = pRecord.MoveBuffer();

				// Check initialization
				GO_SPEECH_RECOGNITION_BOOL initialized = GO_SPEECH_RECOGNITION_IsInitialized();
				if (initialized != GO_SPEECH_RECOGNITION_TRUE || !IsPluginLoaded()) {
					if (_stopping) {
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
					_stopping = true;
					_isSending = false;
					return;
				}
			}
			_isSending = false;
		});

		_tReceiving = std::make_unique<std::thread>([this] {
			_isReceiving = true;
			LogInfo("VoiceInput: Started receiving transcripts.");

			while (!_stopping) {

				std::string receivedString = "";
				char* received;

				// Check initialization
				GO_SPEECH_RECOGNITION_BOOL initialized = GO_SPEECH_RECOGNITION_IsInitialized();
				if (initialized != GO_SPEECH_RECOGNITION_TRUE || !IsPluginLoaded()) {
					if (_stopping) {
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
					VoiceMonitorHandler::instance().setNewText(PrintCategory::CONNECTIONGOOGLE, L"off");
					_stopping = true;
					_isReceiving = false;
					return;
				}

				// Save transcript as a string
				receivedString = received;
				if (!receivedString.empty()) {
					LogInfo("VoiceInput: Received: [" + receivedString + "]");
					VoiceMonitorHandler::instance().setNewText(PrintCategory::LASTWORD, _converter.from_bytes(receivedString));
					// Save transcript in _recognitionResults vector
					std::lock_guard<std::mutex> lock(_transcriptGuard);
					_recognitionResults.push(receivedString);
				}
			}
			_isReceiving = false;
		});
	}
}

void VoiceInput::Reactivate() {
	_tReactivating = std::make_unique<std::thread>([this] {

		std::lock_guard<std::mutex> lock(_reactivateGuard);

		if (_voiceInputState != VoiceInputState::Changing)
			_voiceInputState = VoiceInputState::Restarting;
		this->Deactivate();
		this->Activate();
	});
}


void VoiceInput::Deactivate() {
	_stopping = true;
	LogInfo("VoiceInput: Stopping audio recording and transcribing process.");
	VoiceMonitorHandler::instance().setNewText(PrintCategory::CONNECTIONGOOGLE, L"off");

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
			return;
		}

		// Close stream
		err = Pa_CloseStream(_pInputStream);
		if (err != paNoError)
		{
			LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
			_pInputStream = NULL;
			return;
		}

		// Set stream to NULL
		_pInputStream = NULL;

	}

	while (_isSending || _isReceiving) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	};


	LogInfo("VoiceInput: Stopped audio recording and transcribing process.");


	if (_voiceInputState == VoiceInputState::Active) {
		_voiceInputState = VoiceInputState::Inactive;
		VoiceMonitorHandler::instance().setNewText(PrintCategory::CONNECTIONGOOGLE, L"off");
	}
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
bool ContinuousAudioRecord::AddSample(short sample)
{
	std::lock_guard<std::mutex> lock(_bufferGuard);

	if (!_maximumSizeReached)
	{
		_maximumSizeReached = static_cast<int>(_buffer.size()) == _maximumSize;
		_index = _buffer.size();
	}
	if (_maximumSizeReached)
	{
		_buffer.pop_front();
	}

	_buffer.push_back(sample);
	return true;
}

std::unique_ptr<std::vector<short> > ContinuousAudioRecord::MoveBuffer()
{
	std::lock_guard<std::mutex> lock(_bufferGuard);

	std::vector<short> audioData(_buffer.size());

	std::copy(std::begin(_buffer), std::end(_buffer), std::begin(audioData));

	_buffer.clear();

	return std::make_unique<std::vector<short> >(audioData);
}
