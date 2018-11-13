//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Handles communication with LabStreamingLayer. Input only supports single
// strings, output can be customized.

#ifndef LABSTREAM_H_
#define LABSTREAM_H_

// Include LabStreamingLayer header but stop Visual Studio to display annoying warnings
#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4389)
#endif

#include "externals/liblsl/include/lsl_cpp.h"

#ifdef _WIN32
#pragma warning( pop )
#endif

#include <thread>
#include <mutex>
#include <string>
#include <vector>

// ########################
// ### LAB STREAM INPUT ###
// ########################

class LabStreamInput
{
public:

	// Constructor
	LabStreamInput(std::string streamInputName);

	// Destructor
	virtual ~LabStreamInput();

	// Poll received events (clears events)
	std::vector<std::string> Poll();

private:

	// Private copy / asignment constructors
	LabStreamInput(const LabStreamInput&) {}
	LabStreamInput& operator = (const LabStreamInput &) { return *this; }

	// Members
	std::unique_ptr<std::thread> _upReceiverThread;
	std::mutex _inputMutex;
	std::vector<std::string> _inputBuffer;
};

// #########################
// ### LAB STREAM OUTPUT ###
// #########################

template<typename Type>
class LabStreamOutput
{
public:

	// Constructor
	LabStreamOutput(lsl::stream_info streamInfo)
	{
		// Set up stream outlet
		_upStreamOutlet = std::unique_ptr<lsl::stream_outlet>(new lsl::stream_outlet(streamInfo));
	}

	// Destructor
	virtual ~LabStreamOutput() {}

	// Send event
	void Send(std::vector<Type> data)
	{
		_upStreamOutlet->push_sample(data);
	}

private:

	// Private copy / assignment constructors
	LabStreamOutput(const LabStreamOutput&) {}
	LabStreamOutput& operator = (const LabStreamOutput &) { return *this; }

	// Members
	std::unique_ptr<lsl::stream_outlet> _upStreamOutlet;
};

#endif // LABSTREAM_H_