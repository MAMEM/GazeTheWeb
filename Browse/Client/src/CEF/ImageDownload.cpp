//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/CEF/ImageDownload.h"
#include "src/Utils/Logger.h"

void PendingImageDownload::OnDownloadImageFinished(const CefString& image_url,
	int http_status_code,
	CefRefPtr<CefImage> image)
{
	//LogDebug("PendingImageDownload: Finished image download for url:\n", image_url.ToString());
	_handler->ForwardFaviconBytes(_corresponding_browser, image);
	_handler->FinishImageDownload(this);
}


void HandlerImageInterface::StartImageDownload(CefRefPtr<CefBrowser> browser, CefString img_url)
{
	CefRefPtr<PendingImageDownload> download(new PendingImageDownload(this, browser));
	_downloads.push_back(download);
	//LogDebug("HandlerImageInterface: Starting new image download for url:\n", img_url.ToString());
	browser->GetHost()->DownloadImage(img_url, true, 0, false, download);
}

void HandlerImageInterface::FinishImageDownload(CefRefPtr<PendingImageDownload> download)
{
	const auto& iter = std::find(_downloads.begin(), _downloads.end(), download);
	// Remove download from vector by using its iterator in _downloads
	if (iter != _downloads.end())
	{
		// Removal of single ref-counted instance from vector leads to object's destruction
		_downloads.erase(iter);
	}
}