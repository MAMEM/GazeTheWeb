//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Singleton which sends messages to JavaScript logging mediator.

#ifndef JSMAILER_H_
#define JSMAILER_H_

#include <string>

// Forward declaration
class Handler;

class JSMailer
{
public:

	// Get instance
	static JSMailer& instance()
	{
		static JSMailer _instance;
		return _instance;
	}

	// Destructor
	~JSMailer() {}

	// Set handler (should be only called by handler itself)
	void SetHandler(Handler* pHandler);

	// Send message to JavaScript logging mediator
	void Send(std::string message);

private:

	// Pointer to handler which can call into JavaScript
	Handler* _pHandler = nullptr;

	// Private copy / assignment constructors
	JSMailer() {}
	JSMailer(const JSMailer&) {}
    JSMailer& operator = (const JSMailer &) { return *this; }
};

#endif // JSMAILER_H_