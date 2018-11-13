//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "LabStream.h"
#include <iostream>
#include <chrono>

// ########################
// ### LAB STREAM INPUT ###
// ########################

LabStreamInput::LabStreamInput(std::string streamInputName)
{
    // Setting up receiving
    std::mutex* pMutex = &_inputMutex; // pointer to mutex because reference of private member is not allowed
    std::vector<std::string>* pBuffer = &_inputBuffer; // same for buffer itself
    _upReceiverThread = std::unique_ptr<std::thread>(new std::thread([pMutex, pBuffer, streamInputName]() // pass copies of pointers to members
    {
        // Receive data
        bool connected = false;
        std::vector<lsl::stream_info> streamInfos;
		std::unique_ptr<lsl::stream_inlet> upStreamInlet;
        std::vector<std::string> localBuffer;
        while(true)
        {
            // Build up connection if necessary
            if(!connected)
            {
                streamInfos.clear();
                while (streamInfos.empty())
                {
                    streamInfos = lsl::resolve_stream("name", streamInputName); // search for stream with certain name
                    std::this_thread::sleep_for(std::chrono::seconds(1)); // do it every second
                }

                // Take first stream you can find
                upStreamInlet = std::unique_ptr<lsl::stream_inlet>(new lsl::stream_inlet(streamInfos[0]));
                connected = true;
            }

            // Listen for input
            try
            {
				upStreamInlet->pull_sample(localBuffer);
            }
            catch(lsl::lost_error e)
            {
                // Connection lost, do connection again
                connected = false;
                continue;
            }

            // Write it to shared memory
            pMutex->lock();
            (*pBuffer).insert(pBuffer->end(), localBuffer.begin(), localBuffer.end()); // append values to shared buffer
            pMutex->unlock();

            // Clear local buffer
            localBuffer.clear();
        }
    }));
}

LabStreamInput::~LabStreamInput()
{
    // Nothing to do
}

std::vector<std::string> LabStreamInput::Poll()
{
    // Used as return value
    std::vector<std::string> input;

    // Read values from thread
    _inputMutex.lock();
    input = _inputBuffer;
    _inputBuffer.clear(); // clear shared buffer
    _inputMutex.unlock();

    // Return received values
    return input;
}