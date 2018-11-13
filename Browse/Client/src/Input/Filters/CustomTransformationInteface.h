//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Abstract interface for custom transformation for eye gaze filters. See
// declaration in Filter.h for documentation.

#ifndef CUSTOMTRANSFORMATIONINTERFACE_H_
#define CUSTOMTRANSFORMATIONINTERFACE_H_

#include <functional>

// Typedef for custom transformation
typedef std::function<void(double&, double&)> FilterTransformation;

// Interface
class CustomTransformationInterface
{
public:
	virtual bool RegisterCustomTransformation(std::string name, FilterTransformation transformation) = 0;																			 
	virtual bool ChangeCustomTransformation(std::string name, FilterTransformation transformation) = 0;
	virtual bool UnregisterCustomTransformation(std::string name) = 0;
	virtual double GetFilteredGazeX(std::string name) const = 0;
	virtual double GetFilteredGazeY(std::string name) const = 0;
};

#endif // CUSTOMTRANSFORMATIONINTERFACE_H_
