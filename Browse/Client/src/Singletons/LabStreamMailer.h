//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Singleton to send messages into LabStreamingLayer and receive incoming
// ones. A receiver has to create a LabStreamCallback object and register
// it in the mailer. This saves one from unregistering callbacks, because
// weak pointers are used.

#ifndef LABSTREAMMAILER_H_
#define LABSTREAMMAILER_H_

#include "common/LabStream/LabStream.h"
#include <functional>

// Class to abstract callbacks when message is received
class LabStreamCallback
{
public:

	// Constructor taking function that is called at callback
	LabStreamCallback(std::function<void(std::vector<std::string>)> callbackFunction)
	{
		_callbackFunction = callbackFunction;
	}

private:

	// LabStreamMailer may send messages to this
	friend class LabStreamMailer;

	// Receive message from lab streaming layer
	void Receive(std::vector<std::string> messages) const
	{
		_callbackFunction(messages);
	}

	// Function to callback
	std::function<void(std::vector<std::string>)> _callbackFunction;
};

// Acutal class
class LabStreamMailer
{
public:

	// Get instance
	static LabStreamMailer& instance()
	{
		static LabStreamMailer _instance;
		return _instance;
	}

	// Destructor
	~LabStreamMailer() {}

	// Send message
	void Send(std::string message);

	// Someone has to poll this so new messages are read and sent to callbacks. Should be done by master.
	void Update();

	// Register callback to receive messages. If weak pointer is invalid, callback is removed
	void RegisterCallback(std::weak_ptr<LabStreamCallback> wpCallback);

private:

	// LabStreamingLayer connection
	std::unique_ptr<LabStreamInput> _upLabStreamInput;
	std::unique_ptr<LabStreamOutput<std::string> > _upLabStreamOutput;

	// Vector of registered callbacks
	std::vector<std::weak_ptr<LabStreamCallback> > _callbacks;

	// Private copy / assignment constructors
	LabStreamMailer();
	LabStreamMailer(const LabStreamMailer&) {}
    LabStreamMailer& operator = (const LabStreamMailer &) { return *this; }
};

#endif // LABSTREAMMAILER_H_