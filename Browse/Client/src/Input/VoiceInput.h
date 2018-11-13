//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Stub for voice input handling.

// TODO @ Christopher: Change this interface as desired!

#ifndef VOICEINPUT_H_
#define VOICEINPUT_H_

enum class VoiceAction
{
	NO_ACTION, SCROLL_UP, SCROLL_DOWN // TODO @ Christopher: change datastructure
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

private:

	// Nothing yet
};

#endif // VOICEINPUT_H_