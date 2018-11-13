//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "EyetrackerData.h"
#include <mutex>

namespace eyetracker_global
{
    // Variables
	auto spSampleDataQueue = SampleQueue(new std::deque<SampleData>);
	std::mutex mutex;
	
	// Wrapper for lab stream output
	struct LabStreamOutputWrapper
	{
	public:
		LabStreamOutputWrapper(lsl::stream_info streamInfo, bool stream) : output(streamInfo), stream(stream) {}
		void Continue() { stream = true; }
		void Pause() { stream = false; }
		void Update(const std::vector<double>& rData) {
			if (stream) { output.Send(rData); } // send data if streaming ok
		}

	private:
		bool stream;
		LabStreamOutput<double> output;
	};
	std::shared_ptr<LabStreamOutputWrapper> spLabStreamOutput = nullptr;

	void SetupLabStream(lsl::stream_info streamInfo)
	{
		mutex.lock(); // lock
		spLabStreamOutput = 
			std::shared_ptr<LabStreamOutputWrapper >(new LabStreamOutputWrapper(
				streamInfo, // stream info given by eye tracker implementation
				true)); // start directly with streaming (TODO: right now in EyeInput it is manually paused if during initialization data transfer was paused. Better ask here what is the state in master than relying on EyeInput class)
		mutex.unlock(); // unlock
	}

	void TerminateLabStream()
	{
		mutex.lock(); // lock
		spLabStreamOutput = nullptr;
		mutex.unlock(); // unlock
	}

	void ContinueLabStream()
	{
		mutex.lock(); // lock
		if (spLabStreamOutput) { spLabStreamOutput->Continue(); }
		mutex.unlock(); // unlock
	}

	void PauseLabStream()
	{
		mutex.lock(); // lock
		if (spLabStreamOutput) { spLabStreamOutput->Pause(); }
		mutex.unlock(); // unlock
	}

    void PushBackSample(SampleData sample) // called by eye tracker thread
    {
		mutex.lock(); // lock

		// Send to lab streaming layer
		if (spLabStreamOutput) { spLabStreamOutput->Update({ sample.x, sample.y }); } // handles pause etc. internally

		// Push sample to queue
		if (sample.valid) // only push valid samples to the queue
		{
			spSampleDataQueue->push_back(sample); // pushes sample
		}

		mutex.unlock(); // unlock
    }

	void FetchSamples(SampleQueue& rspSamples) // called by main thread
    {
		mutex.lock(); // lock
		rspSamples = spSampleDataQueue; // move data to output
		spSampleDataQueue = SampleQueue(new std::deque<SampleData>); // intialize empty queue for new data
		mutex.unlock(); // unlock
    }
}
