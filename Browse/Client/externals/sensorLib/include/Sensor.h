#pragma once
#ifndef SENSOR_H
#define SENSOR_H
#include <stdlib.h>
#include <string>
#include <thread>
#include <atomic>
#include "lsl_c.h"
#include "lsl_cpp.h"
#include <memory>

class recording;
namespace SensorLib
{
	enum SensorDevice { EMOTIV , MYGAZE, SHIMMER};
	enum SensorType {EEG, ET, GSR, MARKERS};
	enum SensorStatus {ERR, WARNING, NOT_CONNECTED, BUSY, CONNECTED, STREAMING, RECORDING};
	class Recorder;
	class SensorLibrary;
	
	class Sensor {
	public:
		Sensor();
		~Sensor();
		virtual SensorStatus getStatus() = 0;
		//connect to LSL
		virtual void connect() = 0;
		//kill LSL
		virtual void disconnect() = 0;
		//record to file
		void statusUpdate(SensorStatus status);
		
		void registerStatusUpdateCallback(SensorLibrary *lib);
		void unregisterStatusUpdateCallback();
		lsl::stream_info getStreamInfo();

		std::string name;
		int numChannels;
		int samplingRate;
		std::shared_ptr<Recorder> recorder;
		recording* currentRecording;
		bool shouldRecord;
		bool shouldShutDown;
		bool threadRunning;
		
	protected:
		SensorType type;
		SensorDevice device;
		std::string filename;
		std::thread* lsl_thread;
		std::atomic<bool> lslrunning;
		std::atomic<SensorStatus> status;
		SensorLibrary* sLib;
		virtual void lsl_worker() = 0;

	};
}
#endif