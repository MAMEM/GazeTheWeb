//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
//============================================================================

#include "MessageRouter.h"
#include "src/CEF/Mediator.h"
#include "src/Utils/Logger.h"
#include "src/CEF/Data/DOMNode.h"
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
	const std::string requestString = request.ToString();

	// ###############
	// ### Favicon ###
	// ###############

	if (requestString == "faviconBytesReady")
	{
		// Logging
		LogDebug("MessageRouter: Received 'faviconBytesReady' callback from Javascript");

		// Tell renderer to read out favicon image's bytes
		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("GetFavIconBytes");
		browser->SendProcessMessage(PID_RENDERER, msg);

		// Success!
		callback->Success("success");
		return true;
	}

	// ######################
	// ### Text Selection ###
	// ######################

	// Text selection callback, asynchronously called when CefMediator::EndTextSelection finishes
	if (requestString.compare(0, 8, "#select#") == 0)
	{
		// Split result
		auto split = SplitBySeparator(requestString, '#');

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
	if (requestString.compare(0, 9, "#fixElem#") == 0)
	{
		std::vector<std::string> data = SplitBySeparator(requestString, '#');
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
				browser->SendProcessMessage(PID_RENDERER, msg);

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

	// ########################
	// ### Overflow Element ###
	// ########################

	if (requestString.compare(0, 9, "#ovrflow#") == 0)
	{
		if (requestString.compare(9, 4, "add#") == 0) // adding overflow element
		{
			std::string dataStr = requestString.substr(13, requestString.length());
			std::vector<std::string> data = SplitBySeparator(dataStr, '#');

			// Extract OverflowElement ID from dataStr
			int id = std::stoi(data[0]);

			// Extract rect data from encoded String
			std::vector<float> rectData;
			std::vector<std::string> rectStrData = SplitBySeparator(data[1], ';');
			std::for_each(
				rectStrData.begin(),
				rectStrData.end(), 
				[&rectData](std::string str) {rectData.push_back(std::stod(str)); }
			);
			Rect rect = Rect(rectData);

			// Extract maximum possible scrolling in x and y direction
			std::vector<std::string> scrollMaxString = SplitBySeparator(data[2], ';');
			if (scrollMaxString[0].compare("undefined") == 0 || scrollMaxString[1].compare("undefined") == 0)
			{
				LogDebug("MsgRouter: Error in OverflowElement creation: Missing max scrolling value(s)! Setting them to 0.");
			}
			int scrollLeftMax = (scrollMaxString[0].compare("undefined") == 0) ? 0 : std::stoi(scrollMaxString[0]);
			int scrollTopMax = (scrollMaxString[1].compare("undefined") == 0) ? 0 : std::stoi(scrollMaxString[1]);

			// Add overflow element
			_pMediator->AddOverflowElement(browser, std::make_shared<OverflowElement>(id, rect, scrollLeftMax, scrollTopMax));

			// Success!
			callback->Success("success");
			return true;
		}
		if (requestString.compare(9, 4, "rem#") == 0) // removing overflow element
		{
			// Remove overflow element
			std::vector<std::string> dataStr = SplitBySeparator(requestString.substr(13), '#');
			_pMediator->RemoveOverflowElement(browser, std::stoi(dataStr[0]));

			// Success!
			callback->Success("success");
			return true;
		}
		if (requestString.compare(9, 4, "upd#") == 0) // update overflow element
		{
			std::vector<std::string> dataStr = SplitBySeparator(requestString.substr(13), '#');

			// Assuming elementId, attribute name & data are enough information (at this time)
			if (dataStr.size() == 3)
			{
				// Overflow Element ID
				int id = std::stoi(dataStr[0]);

				// Overflow Elements attribute as string, which gets updated
				std::string attr = dataStr[1];

				// dataStr[2] contains data for given attributes value changes

				// Get access to given Overflow Element in Tab
				std::weak_ptr<OverflowElement> wpElem = _pMediator->GetOverflowElement(browser, id);
				if (const auto& elem = wpElem.lock())
				{
					// Update Overflow Element's rect data
					if (attr == "rect")
					{
						std::vector<std::string> rectData = SplitBySeparator(dataStr[2], ';');
						std::vector<float> rect;
						// Convert Rect coordinates from string to float
						std::for_each(
							rectData.begin(),
							rectData.end(),
							[&rect](std::string str) {rect.push_back(std::stof(str)); }
						);

						// Use float coordinates to update Rect #0
						elem->UpdateRect(0, std::make_shared<Rect>(rect));
					}

					// Update OverflowElement's fixation status
					if (attr == "fixed")
					{
						elem->UpdateFixation(std::stoi(dataStr[2]));
					}
				}

				// Success!
				callback->Success("success");
				return true;
			}
			else
			{
				// Failure!
				LogError("MsgRouter: An error occured in decoding the update String of an OverflowElement!");
				callback->Failure(-1, "some error occured");
				return true;
			}
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

	if (requestString.compare(0, 4, "DOM#") == 0)
	{
		std::vector<std::string> data = SplitBySeparator(requestString, '#');

		if (data.size() > 3)
		{
			const std::string& op = data[1];
			const int& numeric_type = std::stoi(data[2]);
			const int& id = std::stoi(data[3]);
			
			// Identify type as string with DOMNodeType
			DOMNodeType type;
			switch (numeric_type)
			{
				case(0) : {type = DOMNodeType::TextInput; break;  };
				case(1) : {type = DOMNodeType::TextLink; break;  };
				case(2) : {type = DOMNodeType::SelectField; break;  };
				default: {
					LogError("MsgRouter: Can't process the following incoming msg:\n", requestString, "\nCAUSE: Unknown DOMNodeType: ", numeric_type);
					return true;
				}
			}

			if (op.compare("add") == 0) // adding of DOM node
			{
				// Create blank DOM node of given node type with nodeID
				_pMediator->AddDOMNode(
					browser,
					std::make_shared<DOMNode>(
						type,
						browser->GetMainFrame()->GetIdentifier(),
						id,
						Rect()
						)
					);

				// Send IPC msg to Renderer to fetch more data to fill in
				CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("LoadDOMNodeData");
				msg->GetArgumentList()->SetInt(0, type);
				msg->GetArgumentList()->SetInt(1, id);
				browser->SendProcessMessage(PID_RENDERER, msg);
			}

			if (op.compare("rem") == 0) // removing of DOM node
			{
				_pMediator->RemoveDOMNode(browser, type, id);
			}

			if (op.compare("upd") == 0) // updating of DOM node
			{
				if (data.size() > 5)
				{
					const int& attr = std::stoi(data[4]);
					const std::string& attrData = data[5];
					
					switch (attr)
					{
						// List of Rects were updated
						case(0):
						{
							std::vector<std::string> rectDataStr = SplitBySeparator(attrData, ';');
							std::vector<float> rectData;

							// Extract each float value from string, earlier separated by ';', and convert string to numerical value
							std::for_each(
								rectDataStr.begin(),
								rectDataStr.end(), 
								[&rectData](std::string value) {rectData.push_back(std::stod(value)); }
							);

							// Read out each 4 float values und create 1 Rect with them
							std::vector<Rect> rects;
							for (int i = 0; i + 3 < rectData.size(); i += 4)
							{
								Rect rect = Rect(rectData[i], rectData[i + 1], rectData[i + 2], rectData[i + 3]);
								rects.push_back(rect);
							}

							// Get weak_ptr to target node and get shared_ptr targetNode out of weak_ptr
							if (auto targetNode = _pMediator->GetDOMNode(browser, type, id).lock())
							{
								targetNode->SetRects(std::make_shared<std::vector<Rect>>(rects));
							}
							break;
						};

						// Node's fixation status changed
						case(1):
						{
							bool boolVal = attrData.at(0) != '0';

							// Get weak_ptr to target node and get shared_ptr targetNode out of weak_ptr
							if (auto targetNode = _pMediator->GetDOMNode(browser, type, id).lock())
							{
								targetNode->SetFixed(boolVal);
							}

							break;
						};

						// Node's visibility has changed
						case (2) :
						{
							bool boolVal = attrData.at(0) != '0';

							// Get weak_ptr to target node and get shared_ptr targetNode out of weak_ptr
							if (auto targetNode = _pMediator->GetDOMNode(browser, type, id).lock())
							{
								targetNode->SetVisibility(boolVal);
							}

							break;
						}

						// Node's text content changed
						case (3):
						{
							if (auto targetNode = _pMediator->GetDOMNode(browser, type, id).lock())
							{
								targetNode->SetText(attrData);
							}
							break;
						}

						// Node is set as password input field
						case(4):
						{
							if (auto targetNode = _pMediator->GetDOMNode(browser, type, id).lock())
							{
								targetNode->SetAsPasswordField();
								LogDebug("MsgRouter: Set input field node to password field!");
							}
							break;
						}

						// Fallback
						default:
						{
							LogDebug("MsgHandler: Received Javascript 'update' request of DOMNode attribute=", attr, ", which is not yet defined.");
						}
					}
				}
				else
				{
					LogDebug("MsgRouter: Expected more data in DOMObject update:\n", requestString, "\nAborting update.");
				}
			}
		}
		
		// Success! (TODO: there may be failures which can be passed to JavaScript)
		callback->Success("success");
		return true;
	}

	// If request is unknown & couldn't be handled, assume it to be a ConsolePrint call in JS and log it
	LogDebug("Javascript: ", requestString);

	return false;
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

		// Call callback
		_callbackFunction(message);
		callback->Success("success"); // tell JavaScript about success
		return true;
	}

	return false;
}