//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Used for favicon retrieval

#ifndef IMAGEDOWNLOAD_H_
#define IMAGEDOWNLOAD_H_

#include "include/cef_browser.h"
#include <algorithm>

class HandlerImageInterface;	// Forward declaration

class PendingImageDownload : public CefDownloadImageCallback,
	public CefRefCount
{
public:
	PendingImageDownload(HandlerImageInterface* handler, CefRefPtr<CefBrowser> browser) :
		_handler(handler), _corresponding_browser(browser) {};

	void OnDownloadImageFinished(const CefString& image_url,
		int http_status_code,
		CefRefPtr<CefImage> image) OVERRIDE;

private:
	HandlerImageInterface* _handler;
	CefRefPtr<CefBrowser> _corresponding_browser;

	IMPLEMENT_REFCOUNTING(PendingImageDownload);
};


class HandlerImageInterface
{
public:
	virtual bool ForwardFaviconBytes(CefRefPtr<CefBrowser> browser, CefRefPtr<CefImage> img) = 0;

	void StartImageDownload(CefRefPtr<CefBrowser> browser, CefString img_url);
	void FinishImageDownload(CefRefPtr<PendingImageDownload> download);


private:
	std::vector< CefRefPtr<PendingImageDownload> > _downloads;
};


#endif  // IMAGEDOWNLOAD_H_
