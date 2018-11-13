#pragma once  
#ifndef SENSORLIBRARY_H
#define SENSORLIBRARY_H
#ifdef SENSORLIBRARY_EXPORTS  
#define SENSORLIBRARY_API __declspec(dllexport)   
#else  
#define SENSORLIBRARY_API __declspec(dllimport)   
#endif  
#include <stdlib.h>
#include <memory>
#include "Sensor.h"

namespace SensorLib
{
	class Recorder;
	class SensorLibrary
	{
	public:

		SENSORLIBRARY_API SensorLibrary(void);
		SENSORLIBRARY_API ~SensorLibrary(void);

		//SENSORLIBRARY_API int* getStatus(void);
		SENSORLIBRARY_API void printStatus(void);

		SENSORLIBRARY_API void startRecording(void);
		SENSORLIBRARY_API void stopRecording();
		//SENSORLIBRARY_API void stopRecording(void);

		SENSORLIBRARY_API void shutdownSensors(void);

		void sensorUpdate(Sensor * sensor, SensorStatus status);

		std::shared_ptr<Recorder> recorder;
	private:
		std::vector<Sensor*> sensors;
	};
}
#endif