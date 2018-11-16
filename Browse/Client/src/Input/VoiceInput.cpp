//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "VoiceInput.h"
#include "src/Utils/Logger.h"
#include "map"

// Use portaudio library coming with eyeGUI. Bad practice, but eyeGUI would be needed to be linked dynamically otherwise
#include "submodules/eyeGUI/externals/PortAudio/include/portaudio.h"

// TODO @ Christopher: Please try to use portaudio only in the CPP not, in the header
PaStream* pInputStream = nullptr;

std::map<std::string, VoiceAction> voiceActionMapping = {

	{ "up", VoiceAction::SCROLL_UP },		
	{ "down", VoiceAction::SCROLL_DOWN },	
	{ "top", VoiceAction::TOP },			
	{ "bottom", VoiceAction::BOTTOM },		
	{ "bookmark", VoiceAction::BOOKMARK },	
	{ "back", VoiceAction::BACK },			
	{ "reload", VoiceAction::RELOAD },		
	{ "forward", VoiceAction::FORWARD },	
	{ "new tab",VoiceAction::NEW_TAB },		
	{ "search", VoiceAction::SEARCH },		
	{ "zoom", VoiceAction::ZOOM },			
	{ "tab overview", VoiceAction::TAB_OVERVIEW },
	{ "show bookmarks", VoiceAction::SHOW_BOOKMARKS },
	{ "click", VoiceAction::CLICK },		
	{ "check",VoiceAction::CHECK },			
	{ "play",VoiceAction::PLAY },			
	{ "pause",VoiceAction::PAUSE },			
	{ "mute",VoiceAction::MUTE },			
	{ "unmute",VoiceAction::UNMUTE },		
	{ "text",VoiceAction::TEXT },
	{ "type",VoiceAction::TEXT },
	{ "remove",VoiceAction::REMOVE },		
	{ "clear",VoiceAction::CLEAR },			
	{ "close",VoiceAction::CLOSE },			

};


VoiceInput::VoiceInput()
{
	// TODO

	PaError err;	
	err = Pa_Initialize();
	if (err != paNoError)
	{
		LogError("Could not initialize PortAudio.");
		// TODO exit somehow and avoid any further use of this object during runtime
	}
	else {
		portAudioInitialized = true; 
		LogInfo("PortAudio version: " + std::to_string(Pa_GetVersion()));
	}

	// [INITIALIZING] Load Plugins
	if (portAudioInitialized) {
//		Transcriber transcriptionHandle = Transcriber();
//		transcriptionHandle.run();
	}
}

VoiceInput::~VoiceInput()
{

	// TODO
}

VoiceAction VoiceInput::Update(float tpf)
{
	// TODO




	// Fallback if error occurs
	return VoiceAction::NO_ACTION;
}

void VoiceInput::addTranscript(std::string transcript) {
	std::lock_guard<std::mutex> lock(mutexTranscript);

	recognition_results.push_back(transcript);
}


// TODO recognition_results as queue? 
std::string VoiceInput::getLastTranscript() {
	std::lock_guard<std::mutex> lock(mutexTranscript);
	
	return "";
}

Transcriber::Transcriber() {


	// Try to load plugin
//	plugin_handle = LoadLibrary("go-speech-recognition.dll");


	// Check plugin
	if (!plugin_handle)
	{
		LogError("Could not load plugin.");
		return;
	}

	LogInfo("Loaded successfully go-speech-recognition.dll.");

	InitializeStream = reinterpret_cast<INITIALIZE_STREAM>(GetProcAddress(plugin_handle, "InitializeStream"));
	if (!InitializeStream)
	{
		LogError("Could not load function InitializeStream.");
		return;
	}

	SendAudio = reinterpret_cast<SEND_AUDIO>(GetProcAddress(plugin_handle, "SendAudio"));
	if (!SendAudio)
	{
		LogError("Could not load function SendAudio.");
		return;
	}

	ReceiveTranscript = reinterpret_cast<RECEIVE_TRANSCRIPT>(GetProcAddress(plugin_handle, "ReceiveTranscript"));
	if (!ReceiveTranscript)
	{
		LogError("Could not load function ReceiveTranscript.");
		return;
	}

	GetLog = reinterpret_cast<GET_LOG>(GetProcAddress(plugin_handle, "GetLog"));
	if (!GetLog)
	{
		LogError("Could not load function GetLog.");
		return;
	}

	CloseStream = reinterpret_cast<CLOSE_STREAM>(GetProcAddress(plugin_handle, "CloseStream"));
	if (!CloseStream)
	{
		LogError("Could not load function CloseStream.");
		return;
	}

	IsInitialized = reinterpret_cast<IS_INITIALIZED>(GetProcAddress(plugin_handle, "IsInitialized"));
	if (!IsInitialized)
	{
		LogError("Could not load function IsInitialized.");
		return;
	}

	LogInfo("Loaded successfully all functions from go-speech-recognition.dll.");
}

Transcriber::~Transcriber() {

}

void Transcriber::run() {
	
	isRunning = true;
	stopped = false;

	// [INITIALIZATION]
	initializeStream();
	
	// [SETUP RECORDING]

	VoiceInput::startAudioRecording();
	std::shared_ptr<ContinuousAudioRecord> record = VoiceInput::retrieveAudioRecord();
	ContinuousAudioRecord& recordp = *record.get();

	// [SENDING] // loops till it's stopped by error or calling Transcriber::stop()
	sending_thread = std::unique_ptr<std::thread>(new std::thread([this, &recordp] {
		sendAudio(recordp);
	}));

	// [RECEIVING]
	receiveTranscript(); // loops till it's stopped by error or calling Transcriber::stop()
	
	sending_thread->join();
	isRunning = false;
}

void Transcriber::stop() {
	stopped = true;

	sending_thread->join();
	CloseStream();
}


void Transcriber::initializeStream(){

	GO_SPEECH_RECOGNITION_BOOL success = InitializeStream(language, sample_rate);
	if (success != GO_SPEECH_RECOGNITION_TRUE) {
		std::string log = GetLog();
		LogError("Error:" + log);
	}
}

void Transcriber::sendAudio(ContinuousAudioRecord & record){
	
	while (!stopped) {

		std::this_thread::sleep_for(std::chrono::milliseconds(queryTime));
	
		// Retrieve audio
		std::vector<short> audio_data = record.getBuffer();
	
		// Check initialization
		GO_SPEECH_RECOGNITION_BOOL initialized = IsInitialized();
		if (initialized != GO_SPEECH_RECOGNITION_TRUE) {
			LogError("Stream is not initialized! (SENDING)");
			return;
		}
	
		// Send the audio
		GO_SPEECH_RECOGNITION_BOOL success = SendAudio(audio_data.data(), audio_data.size());
		if (success != GO_SPEECH_RECOGNITION_TRUE) {
			std::string log = GetLog();
			LogError("Error:" + log);
		}
	}

}

	// [RECEIVING]
void Transcriber::receiveTranscript() {

	while (!stopped) {

		// Check initialization
		GO_SPEECH_RECOGNITION_BOOL initialized = IsInitialized();
		if (initialized != GO_SPEECH_RECOGNITION_TRUE) {
			LogError("Stream is not initialized! (RECEIVING)");
			return;
		}

		// Receive the transcript
		std::string received = ReceiveTranscript();
		if (!received.empty()) {
			LogInfo("Received: [" + received + "]");
			VoiceInput::addTranscript(received);
		}
	}
}

// Static callback for PortAudio input stream
static int audioStreamRecordCallback(
	const void* inputBuffer, // buffer for audio input via microphone
	void* outputBuffer, // buffer for audio output via speakers
	unsigned long framesPerBuffer, // counts of frames (count of samples for all channels)
	const PaStreamCallbackTimeInfo* timeInfo, // not used
	PaStreamCallbackFlags flags, // not used
	void* data) // pointer to audio data
{
	// Return value
	PaStreamCallbackResult result = PaStreamCallbackResult::paComplete;

	// Cast input buffer
	short* in = (short*)inputBuffer;

	// Prevent unused variable warnings
	(void)inputBuffer;
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



bool VoiceInput::startAudioRecording()
{
	// Check for PortAudio
	if (!portAudioInitialized)
	{
		// DEBUG
		LogError("PortAudio not initialized!");
		return false;
	}

	// Return false if ongoing recording
	if (pInputStream != nullptr)
	{
		// DEBUG
		LogInfo("Recording still in progress...");
		return false;
	}

	pAudioInput = std::shared_ptr<ContinuousAudioRecord>(new ContinuousAudioRecord(
		AUDIO_INPUT_CHANNEL_COUNT, AUDIO_INPUT_SAMPLE_RATE, static_cast<unsigned int>(audio_recording_time_ms / 1000.0)));


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
	err = Pa_OpenStream(&pInputStream, &parameters, NULL, AUDIO_INPUT_SAMPLE_RATE,
		paFramesPerBufferUnspecified, paClipOff, audioStreamRecordCallback, pAudioInput.get());
	if (err != paNoError)
	{
		LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
		pInputStream = nullptr;
		return false;
	}

	// Start stream
	err = Pa_StartStream(pInputStream);
	if (err != paNoError)
	{
		LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
		pInputStream = nullptr; // possible memory leak?
		return false;
	}

	return true;
}

std::shared_ptr<ContinuousAudioRecord> VoiceInput::retrieveAudioRecord()
{
	return pAudioInput;
}

bool VoiceInput::endAudioRecording()
{
	// Check for PortAudio
	if (!portAudioInitialized)
	{
		return false;
	}

	// Variable to fetch errors
	PaError err;

	LogInfo("Stopping audio input stream.");

	// Aborts stream (stop would wait until buffer finished)
	err = Pa_AbortStream(pInputStream);
	if (err != paNoError)
	{
		LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
		pInputStream = NULL;
		return false;
	}

	// Close stream
	err = Pa_CloseStream(pInputStream);
	if (err != paNoError)
	{
		LogError("PortAudio error: " + std::string(Pa_GetErrorText(err)));
		pInputStream = NULL;
		return false;
	}

	// Set stream to NULL
	pInputStream = NULL;


	// Success
	return true;
}


bool ContinuousAudioRecord::addSample(short sample)
{
	std::lock_guard<std::mutex> enterBuffer(buffer_guard);

	if (!maximum_size_reached)
	{
		maximum_size_reached = static_cast<int>(buffer.size()) == maximum_size;
		mIndex = buffer.size();
	}
	if (maximum_size_reached)
	{
		buffer.pop_front();
	}

	buffer.push_back(sample);
	return true;
}

std::vector<short> ContinuousAudioRecord::getBuffer()
{
	std::lock_guard<std::mutex> enter(buffer_guard);

	auto helper = std::make_shared<std::vector<short>>();
	helper->resize(buffer.size());

	std::copy(std::begin(buffer), std::end(buffer), std::begin(*helper));
	buffer.clear();


	return *std::move(helper).get();
}
