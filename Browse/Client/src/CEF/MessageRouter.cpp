//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
//============================================================================

#include "MessageRouter.h"
#include "src/CEF/Mediator.h"
#include "src/Utils/Logger.h"
#include "src/CEF/Data/DOMNode.h"
#include "src/CEF/Data/DOMExtraction.h"
#include <cstdlib>
#include <algorithm>

MessageRouter::MessageRouter(Mediator* pMediator)
{
	// Store pointer to mediator
	_pMediator = pMediator;

	// Create configuration for CEF message router
	CefMessageRouterConfig config;
	config.js_query_function = "cefQuery";
	config.js_cancel_function = "cefQueryCancel";
	_router = CefMessageRouterBrowserSide::Create(config);

	// Add the default handler for messages to the delegated router
	CefMessageRouterBrowserSide::Handler* defaultHandler = new DefaultMsgHandler(_pMediator);
	_router->AddHandler(defaultHandler, true);
}

bool DefaultMsgHandler::OnQuery(CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame,
	int64 query_id,
	const CefString& request,
	bool persistent,
	CefRefPtr<Callback> callback)
{
	const std::string& requestStr = request.ToString();
	std::vector<std::string> split_request = SplitBySeparator(requestStr, '#');

	if (split_request.size() == 3 && split_request[0].compare("resolution") == 0)
	{
		try
		{
			const double& width = std::stod(split_request[1]);
			const double& height = std::stod(split_request[2]);
			_pMediator->ReceivePageResolution(browser, width, height);
		}
		catch (const std::exception& e)
		{
			LogInfo("MsgRouter: Received wrongly typed page resolution information!");
			LogInfo("Caught exception: ", e.what());
		}
		return true;
	}

	// Calls the scroll offset change callback because CEF does so no more: https://bitbucket.org/chromiumembedded/cef/issues/2810
	if (split_request.size() == 3 && split_request[0].compare("scroll") == 0)
	{
		try
		{
			const double& x = std::stod(split_request[1]);
			const double& y = std::stod(split_request[2]);
			_pMediator->OnScrollOffsetChanged(browser, x, y);
		}
		catch (const std::exception& e)
		{
			LogInfo("MsgRouter: Received wrongly typed page scrolling information!");
			LogInfo("Caught exception: ", e.what());
		}
		return true;
	}

	// Receive META data
	if (split_request.size() == 3 && split_request[0].compare("meta") == 0)
	{
		if (split_request[1].compare("keywords") == 0)
		{
			_pMediator->SetMetaKeywords(browser, split_request[2]);
			callback->Success("Received meta keywords");
			return true;
		}
		return false;
	}


	// ###############
	// ### Favicon ###
	// ###############
	if (requestStr.compare(0, 12, "#FaviconURL#") == 0)
	{
		auto split = SplitBySeparator(requestStr, '#');

		if (split.size() > 1)
		{
			// Currently, all finished image downloads trigger set up as favicon image in Tab
			_pMediator->StartImageDownload(browser, split[1]);
		}
		return true;
	}

	if (requestStr == "faviconBytesReady")
	{
		// Logging
		LogDebug("MessageRouter: Received 'faviconBytesReady' callback from Javascript");

		// Tell renderer to read out favicon image's bytes
		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("GetFavIconBytes");
		browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);

		// Success!
		callback->Success("success");
		return true;
	}

	// ######################
	// ### Text Selection ###
	// ######################

	// Text selection callback, asynchronously called when CefMediator::EndTextSelection finishes
	if (requestStr.compare(0, 8, "#select#") == 0)
	{
		// Split result
		auto split = SplitBySeparator(requestStr, '#');

		// Check whether result is empty
		if (split.size() > 1)
		{
			// Extract selection
			const std::string selectionStr = split.at(1);

			// Logging
			LogDebug("MsgRouter: Selected Text: '", selectionStr, "'");

			// Set clipboard in mediator (TODO: what happens when two parallel string extractions are ongoing? some weird overwriting)
			_pMediator->SetClipboardText(selectionStr);
		}
	
		// Success!
		callback->Success("success");
		return true;
	}

	// #####################
	// ### Fixed Element ###
	// #####################

	// Fixed element callbacks
	if (requestStr.compare(0, 9, "#fixElem#") == 0)
	{
		std::vector<std::string> data = SplitBySeparator(requestStr, '#');
		const std::string& op = data[1];

		if (data.size() > 2)
		{
			const std::string& id = data[2];

			if (op.compare("rem") == 0) // removing fixed element
			{
				// Notify Tab via Mediator that a fixed element was removed
				_pMediator->RemoveFixedElement(browser, atoi(id.c_str()));

				// Success!
				callback->Success("success");
				return true;
			}
			if (op.compare("add") == 0) // adding fixed element
			{
				// Tell RenderProcessHandler to read out bounding rectangle coordinates belonging to the given ID
				CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("FetchFixedElements");
				msg->GetArgumentList()->SetInt(0, atoi(id.c_str()));
				browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);

				// Success!
				callback->Success("success");
				return true;
			}
		}
		else
		{
			// Failure!
			LogDebug("MsgRouter: Expected more arguments for fixed elements. Aborting.");
			callback->Failure(-1, "not enough arguments");
			return true;
		}
	}

	// ################
	// ### DOM Node ###
	// ################
	// DOM#{add | rem | upd}#nodeType#nodeID{#attribute#data}#
	// Definitions:
	//	add--> send process message to Renderer to fetch V8 data
	//	rem--> request removal of given node
	//	upd--> update data of an specific node attribute
	//	nodeType: int
	//		0 : TextInput
	//		1 : TextLink
	//	nodeID : int
	//	attribute : int
	//		0 : Rect
	//		1 : _fixed
	//	data : depends on attribute
	// Example: DOM#upd#7#1337#0#0.9;0.7;0.5;0.7#

	// Debug extension: Request C++ sided DOM attribute information
	if (requestStr.compare(0, 10, "DOMAttrReq") == 0)
	{
		LogInfo("DOMAttrReq: Received.");

		std::vector<std::string> data = SplitBySeparator(requestStr, '#');
		if (data.size() > 3)
		{
			const std::string& typeStr = data[1];
			const std::string& idStr = data[2];
			const std::string& attrStr = data[3];

			const int& id = std::stoi(idStr);
			const int& type = std::stoi(typeStr);
			const DOMAttribute& attr = static_cast<DOMAttribute>(std::stoi(attrStr));

			std::weak_ptr<DOMNode> target;
			switch (type)
			{
			case(0): {target = _pMediator->GetDOMTextInput(browser, id); break; }
			case(1): {target = _pMediator->GetDOMLink(browser, id); break; }
			case(2): {target = _pMediator->GetDOMSelectField(browser, id); break; }
			case(3): {target = _pMediator->GetDOMOverflowElement(browser, id); break; }
			case(4): {target = _pMediator->GetDOMVideo(browser, id); break; }
			default: {
				LogError("MsgRouter: DOMAttrReq - Unknown type of DOMNode! type=", type);
			}
			}

			if (auto node = target.lock())
			{
				node->PrintAttribute(attr);
			}
			else
			{
				LogError("MessageRouter: Couldn't access corresponding DOM object with id=", id, " & type=", type);
			}

			callback->Success("success");
		}
		else
			callback->Failure(-1, "Not enough data provided for DOMAttrReq.");

		return true;
	}

	if (requestStr.compare(0, 3, "DOM") == 0)
	{
		std::vector<std::string> data = SplitBySeparator(requestStr, '#');

		if (data.size() > 3)
		{
			const std::string& op = data[1];
			const std::string& typeStr = data[2];
			const std::string& idStr = data[3];
			if (typeStr == "undefined" || typeStr == "null" || idStr == "undefined" || idStr == "null")
			{
				LogError("MsgRouter: Can not fetch node with type: ", typeStr, " and id: ", idStr, ". Aborting.");
				return true;
			}
			const int& type = std::stoi(data[2]);
			const int& id = std::stoi(data[3]);

			// ADDING DOMNODE
			if (op.compare("add") == 0) // adding of DOM node
			{
				// Create blank node object in corresponding Tab object
				switch (type)
				{
					case(0): {_pMediator->AddDOMTextInput(browser, id); break; }
					case(1): {_pMediator->AddDOMLink(browser, id); break; }
					case(2): {_pMediator->AddDOMSelectField(browser, id); break; }
					case(3): {_pMediator->AddDOMOverflowElement(browser, id); break; }
					case(4): {_pMediator->AddDOMVideo(browser, id); break; }
					case(5): {_pMediator->AddDOMCheckbox(browser, id); break; }
					default: {
						LogError("MsgRouter: Adding DOMNode - Unknown type of DOMNode! type=", type);
					}
				}

				// TODO: This could be done in DOMExtraction
				const std::vector<std::string> names = { "TextInput", "Link", "SelectField", "OverflowElement", "Video", "Checkbox" };
				if (type >= names.size() || type < 0)
				{
					LogError("MsgRouter: - ERROR: Unknown numeric DOM node type value: ", type);
					return true;
				}
				const std::string& ipcName = names[type];

				// Instruct Renderer Process to initialize empty DOM Nodes with data
				// TODO: Move this to DOM node constructor?
				CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("LoadDOM" + ipcName + "Data");
				msg->GetArgumentList()->SetInt(0, type);
				msg->GetArgumentList()->SetInt(1, id);
				browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);
			}

			// REMOVE DOMNODE
			if (op.compare("rem") == 0)
			{

				switch (type)
				{
					case(0): {_pMediator->RemoveDOMTextInput(browser, id); break; }
					case(1): {_pMediator->RemoveDOMLink(browser, id); break; }
					case(2): {_pMediator->RemoveDOMSelectField(browser, id); break; }
					case(3) : {_pMediator->RemoveDOMOverflowElement(browser, id); break; }
					case(4): {_pMediator->RemoveDOMVideo(browser, id); break; }
					case(5): {_pMediator->RemoveDOMCheckbox(browser, id); break; }
					default: {
						LogError("MsgRouter: Removing DOMNode - Unknown type of DOMNode! type=", type);
					}
				}
			}

			// UPDATE DOMNODE
			if (op.compare("upd") == 0)
			{
				std::weak_ptr<DOMNode> target;
				switch (type)
				{
					case(0): {target = _pMediator->GetDOMTextInput(browser, id); break; }
					case(1): {target = _pMediator->GetDOMLink(browser, id); break; }
					case(2): {target = _pMediator->GetDOMSelectField(browser, id); break; }
					case(3): {target = _pMediator->GetDOMOverflowElement(browser, id); break; }
					case(4): {target = _pMediator->GetDOMVideo(browser, id); break; }
					case(5): {target = _pMediator->GetDOMCheckbox(browser, id); break; }
					default: {
						LogError("MsgRouter: Updating DOMNode - Unknown type of DOMNode! type=", type);
					}
				}

				if (data.size() > 5)
				{
					// See DOMAttribute.h enum DOMAttribute for numeric interpretation
					const DOMAttribute& attr = (DOMAttribute) std::stoi(data[4]);
					const std::string& attrData = data[5];


					// Perform node update
					bool success = false;
					if (auto node = target.lock())
					{
						success = node->Update(
							(DOMAttribute) attr,
							StringToCefListValue::ExtractAttributeData((DOMAttribute) attr, attrData)
						);
					}
					else
					{
						LogError("MsgRouter: Failed to update node with type: ", type, " and id: ", id, " stored in"\
							" Tab with id: ", browser->GetIdentifier(), ")");
						return true;
					}
					
					if (!success)
					{
						LogError("MsgRouter: Update failed! Node type: ", type, ", node id: ", id, 
							", DOMAttribute: ", DOMAttrToString(attr));
					}

				}
				else
				{
					LogDebug("MsgRouter: Expected more data in DOMObject update:\n", requestStr, "\nAborting update.");
				}
			}

		}
	

		// Success! (TODO: there may be failures which can be passed to JavaScript)
		callback->Success("success");
		return true;
	}

	// If request is unknown & couldn't be handled, assume it to be a ConsolePrint call in JS and log it
	LogInfo("Javascript: ", requestStr);

	return true; // do not forward the message to further processing
}

bool CallbackMsgHandler::OnQuery(CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame,
	int64 query_id,
	const CefString& request,
	bool persistent,
	CefRefPtr<Callback> callback)
{
	const std::string requestString = request.ToString();
	
	// Compare prefix of request with given prefix
	if (requestString.substr(0, _prefix.size()).compare(_prefix) == 0)
	{
		// Remove prefix
		std::string message = requestString.substr(_prefix.size());

		try
		{
			// Call callback
			_callbackFunction(message);
			callback->Success("success"); // tell JavaScript about success
		}
		catch (const std::exception& e)
		{
			LogInfo("CallbackMsgHandler: Exception occured!\n", e.what());
			callback->Failure(0, e.what());
		}
		return true;
	}

	return false;
}