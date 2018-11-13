//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Used for ad blocking.

#ifndef REQUESTHANDLER_H_
#define REQUESTHANDLER_H_

#include "include/cef_request_handler.h"


class RequestHandler : public CefRequestHandler
{
public:

	// Set status
	void BlockAds(bool value) { _blockAds = value; }

	// Called before resource is loaded
	CefRequestHandler::ReturnValue OnBeforeResourceLoad(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request,
		CefRefPtr<CefRequestCallback> callback) OVERRIDE;

private:

    // Include CEF'S default reference counting implementation
    IMPLEMENT_REFCOUNTING(RequestHandler);

	bool _blockAds = true;
};

#endif  // REQUESTHANDLER_H_
