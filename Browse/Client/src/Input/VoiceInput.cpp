//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "VoiceInput.h"
#include "src/Utils/Logger.h"

// Use portaudio library coming with eyeGUI. Bad practice, but eyeGUI would be needed to be linked dynamically otherwise
#include "submodules/eyeGUI/externals/PortAudio/include/portaudio.h"

// TODO @ Christopher: Please try to use portaudio only in the CPP not, in the header

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
	LogInfo("PortAudio version: " + std::to_string(Pa_GetVersion()));
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