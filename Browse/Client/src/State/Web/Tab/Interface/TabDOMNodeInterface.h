//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Müller (muellerd@uni-koblenz.de)
//============================================================================
// Provide outgoing communication for DOM node objects via its corresponding
// Tab parent.
// TODO: strange that some methods are implemented by the interface.

#ifndef TABDOMNODEINTERFACE_H_
#define TABDOMNODEINTERFACE_H_

#include "src/State/Web/Tab/Interface/TabActionInterface.h"
#include "include/cef_browser.h" // TODO: very bad to have this included here.
#include <memory>

class DOMBaseInterface;	// forward declaration

class TabDOMNodeInterface 
{
public:
	// No params given for execution of function
	bool ExecuteCorrespondingJavascriptFunction(std::shared_ptr<DOMBaseInterface> spNode, std::string function);

	// Arbitrary number of arbitrarily typed parameters for calling function
	template<typename T, typename... Args>
	bool ExecuteCorrespondingJavascriptFunction(std::shared_ptr<DOMBaseInterface> spNode, std::string func_name, T param, Args... args)
	{
		CefRefPtr<CefListValue> params = CefListValue::Create();
		AddToList<T>(params, param, args...);

		CefRefPtr<CefProcessMessage> msg = SetupExecuteFunctionMessage(
			std::move(spNode), func_name, params);
		return SendProcessMessageToRenderer(msg);
	}

	virtual bool EmulateKeyboardStrokes(std::string textt) = 0;
	virtual bool EmulateSelectAll() = 0;
	virtual bool EmulateEnterKey() = 0;

private:

	virtual bool SendProcessMessageToRenderer(CefRefPtr<CefProcessMessage> msg) = 0;

	// Maps given data type to CefListValue function in order to set idx to this value
	template<typename T>
	void AppendToList(CefRefPtr<CefListValue> list, T val) { LogInfo("TabDOMNodeInterface: Unhandled data type as input!"); };

	void AppendToList(CefRefPtr<CefListValue> list, bool val) { list->SetBool(list->GetSize(), val); };
	void AppendToList(CefRefPtr<CefListValue> list, double val) { list->SetDouble(list->GetSize(), val); };
	void AppendToList(CefRefPtr<CefListValue> list, float val) { list->SetDouble(list->GetSize(), val); };
	void AppendToList(CefRefPtr<CefListValue> list, int val) { list->SetInt(list->GetSize(), val); };
	void AppendToList(CefRefPtr<CefListValue> list, std::string val) { list->SetString(list->GetSize(), val); };

	template<typename T>
	void AppendToList(CefRefPtr<CefListValue> list, std::vector<T> val)
	{
		CefRefPtr<CefListValue> sublist = CefListValue::Create();
		for (const auto subval : val)
			AppendToList(sublist, subval);
		list->SetList(list->GetSize(), sublist);
	};

	// Add an arbitrarily typed value to a CefListValue
	template<typename T>
	void AddToList(CefRefPtr<CefListValue> list, T val)
	{
		AppendToList(list, val);
	};

	// Add several arbitrarily typed values to a CefListValue
	template<typename T, typename... Args>
	void AddToList(CefRefPtr<CefListValue> list, T val, Args... args)
	{
		AppendToList(list, val);
		AddToList(list, args...);
	}

	CefRefPtr<CefProcessMessage> SetupExecuteFunctionMessage(
		std::shared_ptr<DOMBaseInterface> spNode, std::string func_name, CefRefPtr<CefListValue> param);
};

#endif // TABDOMNODEINTERFACE_H_