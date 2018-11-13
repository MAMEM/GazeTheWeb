//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Müller (muellerd@uni-koblenz.de)
//============================================================================

#include "src/State/Web/Tab/Tab.h"
#include "src/CEF/Mediator.h"
#include "src/CEF/Data/DOMNode.h"

bool TabDOMNodeInterface::ExecuteCorrespondingJavascriptFunction(std::shared_ptr<DOMBaseInterface> spNode, std::string function)
{
	CefRefPtr<CefListValue> params = CefListValue::Create();
	CefRefPtr<CefProcessMessage> msg = SetupExecuteFunctionMessage(std::move(spNode), function, params);
	return SendProcessMessageToRenderer(msg);
}

bool Tab::EmulateKeyboardStrokes(std::string text)
{
	return _pCefMediator->EmulateKeyboardStrokes(this, text);
}

bool Tab::EmulateSelectAll()
{
	return _pCefMediator->EmulateSelectAll(this);
}

bool Tab::EmulateEnterKey()
{
	return _pCefMediator->EmulateEnterKey(this);
}

CefRefPtr<CefProcessMessage> TabDOMNodeInterface::SetupExecuteFunctionMessage(
	std::shared_ptr<DOMBaseInterface> spNode, std::string func_name, CefRefPtr<CefListValue> param)
{
	CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("ExecuteJavascriptFunction");
	const auto& args = msg->GetArgumentList();
	CefRefPtr<CefListValue> header = CefListValue::Create();
	header->SetString(0, func_name);
	header->SetInt(1, spNode->GetType());
	header->SetInt(2, spNode->GetId());

	args->SetList(0, header);
	args->SetList(1, param);
	return msg;
}
