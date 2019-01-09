#pragma once

/*
Author: Christopher Dreide(https://github.com/Drizzy3D)

This Header is needed to use the functions provided by the go-speech-recognition.dll.

See the README.md for instructions on how to use this header.
*/

/*
Create enum, which is needed to handle the return values of some functions as cgo
doesn't support bool transfer between C and Go.
*/
enum GO_SPEECH_RECOGNITION_BOOL {
	GO_SPEECH_RECOGNITION_TRUE = 1,
	GO_SPEECH_RECOGNITION_FALSE = 0
};

/*
GO_SPEECH_RECOGNITION_BOOL InitializeStream(char* cTranscriptLanguage, int cSampleRate):
one time initialization,
sets the streaming session up (saved in global variables),
sends the initial configuration message

Parameter:
cTranscriptLanguage
(transcription language as a string/char* (use BCP-47 language tag))
cSampleRate
(the sample rate of the audio recording as an integer value, it's recommended
use at least 16kHz)
cTranscriptionModel
(the requested model to use: can be either "video", "phone_call", "command_and_search", "default" (see https://cloud.google.com/speech-to-text/docs/basics))

Return:
GO_SPEECH_RECOGNITION_TRUE if successful
GO_SPEECH_RECOGNITION_FALSE if failed (error log can be retrieved with "GetLog()")
*/
typedef GO_SPEECH_RECOGNITION_BOOL(*GO_SPEECH_RECOGNITION_INITIALIZE_STREAM)(char* cTranscriptLanguage, int cSampleRate, char* cTranscriptionModel);

/*
GO_SPEECH_RECOGNITION_BOOL SendAudio(const short* recording, int recording_size):
prepares the inputted audio data to be sent to google,
handles the sending process

Parameters:
recording:
has to be a Pointer to short values representing the audio stream

recording_size:
the size of the transferred recording

Return:
GO_SPEECH_RECOGNITION_TRUE if successful
GO_SPEECH_RECOGNITION_FALSE if failed (error log can be retrieved with "GetLog()")
*/
typedef GO_SPEECH_RECOGNITION_BOOL(*GO_SPEECH_RECOGNITION_SEND_AUDIO)(const short* recording, int recording_size);

/*
ReceiveTranscript (char**):
retrieves and saves the current final transcripts from Google

Return:
(per reference [char* (current transcript)])
GO_SPEECH_RECOGNITION_TRUE if successful
GO_SPEECH_RECOGNITION_FALSE if failed (error log can be retrieved with "GetLog()")
*/
typedef GO_SPEECH_RECOGNITION_BOOL(*GO_SPEECH_RECOGNITION_RECEIVE_TRANSCRIPT)(char**);

/*
char* GetLog ():
returns the last logged event as a String/char*

Return:
char* (last logged event)
*/
typedef char*(*GO_SPEECH_RECOGNITION_GET_LOG)();


/*
void CloseStream ():
closes the streaming session, all accesses to the streaming object
in the go-speech-recognition.dll are secured by mutex
*/
typedef GO_SPEECH_RECOGNITION_BOOL(*GO_SPEECH_RECOGNITION_CLOSE_STREAM)();

/*
GO_SPEECH_RECOGNITION_BOOL IsInitialized ():
returns the status of initialization

Return:
GO_SPEECH_RECOGNITION_TRUE if the stream is initialized
GO_SPEECH_RECOGNITION_FALSE if the stream is not initialized
*/
typedef GO_SPEECH_RECOGNITION_BOOL(*GO_SPEECH_RECOGNITION_IS_INITIALIZED)();
