//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "LabStreamMailer.h"
#include "src/Setup.h"

LabStreamMailer::LabStreamMailer() :
	_upLabStreamInput(std::unique_ptr<LabStreamInput>(new LabStreamInput(setup::LAB_STREAM_INPUT_NAME))),
	_upLabStreamOutput(std::unique_ptr<LabStreamOutput<std::string> >(
		new LabStreamOutput<std::string>(
			lsl::stream_info(
				setup::LAB_STREAM_OUTPUT_NAME, // name
				"Markers", // type
				1, // channel count
				lsl::IRREGULAR_RATE, // rate
				lsl::cf_string, // channel format
				setup::LAB_STREAM_OUTPUT_SOURCE_ID)))) // source id
	
{
	// Nothing to do	
}

void LabStreamMailer::Send(std::string message)
{
	_upLabStreamOutput->Send({ message });
}

void LabStreamMailer::Update()
{
	// Poll incoming messages
	auto messages = _upLabStreamInput->Poll();

	// Go over callbacks and use them, if messages are available
	if (!messages.empty())
	{
		std::vector<int> toBeRemoved;
		int i = 0;
		for (auto& rwpCallback : _callbacks)
		{
			if (auto spCallback = rwpCallback.lock())
			{
				spCallback->Receive(messages);
			}
			else
			{
				// Weak pointer got invalid, so remove it later
				toBeRemoved.push_back(i);
			}
			++i; // increment index
		}

		// Remove dead weak pointers
		for (int j = toBeRemoved.size() - 1; j >= 0; --j) // do it backwards
		{
			_callbacks.erase(_callbacks.begin() + toBeRemoved.at(j));
		}
	}
}

void LabStreamMailer::RegisterCallback(std::weak_ptr<LabStreamCallback> wpCallback)
{
	_callbacks.push_back(wpCallback);
}