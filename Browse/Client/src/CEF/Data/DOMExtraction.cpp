//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "DOMExtraction.h"
#include "src/Utils/Helper.h"

const CefRefPtr<CefListValue> V8ToCefListValue::NestedListOfDoubles(CefRefPtr<CefV8Value> v8rects)
{
	if (v8rects->IsUndefined() || v8rects->IsNull() || !v8rects->IsArray())
		return CefRefPtr<CefListValue>();

	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	CefRefPtr<CefListValue> rects = CefListValue::Create();
	for (int i = 0; i < v8rects->GetArrayLength(); i++)
	{
		CefRefPtr<CefV8Value> v8rect = v8rects->GetValue(i);
		if (v8rect->IsUndefined() || v8rect->IsNull() || !v8rect->IsArray())
			return CefRefPtr<CefListValue>();
		
		CefRefPtr<CefListValue> rect = CefListValue::Create();
		for (int j = 0; j < v8rect->GetArrayLength(); j++)
		{
			rect->SetDouble(j, v8rect->GetValue(j)->GetDoubleValue());
		}
		rects->SetList(i, rect);
	}

	wrapper->SetList(0, rects);
	return wrapper;
}

const CefRefPtr<CefListValue> V8ToCefListValue::ListOfStrings(CefRefPtr<CefV8Value> attrData)
{
	if (!attrData->IsArray())
		return CefRefPtr<CefListValue>();

	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	CefRefPtr<CefListValue> list = CefListValue::Create();
	for (int i = 0; i < attrData->GetArrayLength(); i++)
	{
		if (!attrData->GetValue(i)->IsString())
			list->SetString(i, "");
		else 
			list->SetString(i, attrData->GetValue(i)->GetStringValue());
	}
	wrapper->SetList(0, list);
	return wrapper;
}

const CefRefPtr<CefListValue> V8ToCefListValue::ListOfIntegers(CefRefPtr<CefV8Value> attrData)
{
	if (!attrData->IsArray())
		return CefRefPtr<CefListValue>();

	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	CefRefPtr<CefListValue> list = CefListValue::Create();
	for (int i = 0; i < attrData->GetArrayLength(); i++)
	{
		if (!attrData->GetValue(i)->IsInt())
			list->SetInt(i, -1);	// TODO: Not the best choice for a default value
		else
			list->SetInt(i, attrData->GetValue(i)->GetIntValue());
	}
	wrapper->SetList(0, list);
	return wrapper;
}

const CefRefPtr<CefListValue> V8ToCefListValue::ListOfBools(CefRefPtr<CefV8Value> attrData)
{
	if (!attrData->IsArray())
		return CefRefPtr<CefListValue>();

	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	CefRefPtr<CefListValue> list = CefListValue::Create();
	for (int i = 0; i < attrData->GetArrayLength(); i++)
	{
		if (!attrData->GetValue(i)->IsBool())
			list->SetBool(i, false);
		else
			list->SetBool(i, attrData->GetValue(i)->GetBoolValue());
	}
	wrapper->SetList(0, list);
	return wrapper;
}

const CefRefPtr<CefListValue> V8ToCefListValue::Boolean(CefRefPtr<CefV8Value> attrData)
{
	if (!attrData->IsBool())
		return CefRefPtr<CefListValue>();

	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	wrapper->SetBool(0, attrData->GetBoolValue());
	return wrapper;
}

const CefRefPtr<CefListValue> V8ToCefListValue::Integer(CefRefPtr<CefV8Value> attrData)
{

	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	if (attrData->IsNull() || attrData->IsUndefined() || !attrData->IsInt()) // TODO: Check everywhere for null and undefined!
		wrapper->SetInt(0, -1);
	else
		wrapper->SetInt(0, attrData->GetIntValue());
	
	return wrapper;
}

const CefRefPtr<CefListValue> V8ToCefListValue::String(CefRefPtr<CefV8Value> attrData)
{
	if (!attrData->IsString())
		return CefRefPtr<CefListValue>();

	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	wrapper->SetString(0, attrData->GetStringValue());
	return wrapper;
}

const void V8ToCefListValue::_Log(std::string txt, CefRefPtr<CefBrowser> browser)
{
	if (browser != nullptr)
	{
		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("IPCLog");
		msg->GetArgumentList()->SetBool(0, true);
		msg->GetArgumentList()->SetString(1, txt);
		browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, msg);
	}
}

const CefRefPtr<CefListValue> V8ToCefListValue::ExtractAttributeData(DOMAttribute attr, CefRefPtr<CefV8Value> obj, CefRefPtr<CefBrowser> browser)
{
	if (obj->IsNull() || obj->IsUndefined())
		return CefRefPtr<CefListValue>();

	// Early exit
	if (AttrGetter.find(attr) == AttrGetter.end() || AttrConversion.find(attr) == AttrConversion.end())
	{
		_Log("V8ToCefListValue conversion: Insufficient information defined for DOMAttribute " + std::to_string((int)attr) + "! Abort.", browser);
		return CefRefPtr<CefListValue>();
	}

	// Return getter function,
	CefRefPtr<CefV8Value> getter = obj->GetValue(AttrGetter.at(attr));

	// Check if getter function is valid
	if (getter->IsUndefined() || getter->IsNull() || !getter->IsFunction())
	{
		_Log("V8ToCefListValue conversion: Could not access getter function '"+AttrGetter.at(attr)+
			"' for DOMAttribute " + std::to_string((int)attr) + " in Javascript object.", browser);
		return CefRefPtr<CefListValue>();
	}

	// Execute getter function
	CefRefPtr<CefV8Value> data = getter->ExecuteFunction(obj, {});

	if (data->IsUndefined() || data->IsNull())
		return CefRefPtr<CefListValue>();

	// Convert returned V8Value to CefListValue
	return AttrConversion.at(attr)(data);
}

const CefRefPtr<CefListValue> StringToCefListValue::NestedListOfDoubles(std::string attrData)
{
	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	CefRefPtr<CefListValue> extracted_data = CefListValue::Create();
	std::vector<std::string> rectDataStr = SplitBySeparator(attrData, ';');
	std::vector<double> rectData;

	// Extract each float value from string, earlier separated by ';', and convert string to numerical value
	std::for_each(
		rectDataStr.begin(),
		rectDataStr.end(),
		[&rectData](std::string value) {rectData.push_back(std::stod(value)); }
	);

	// Read out each 4 float values und create 1 Rect with them
	for (int i = 0; i + 3 < (int)rectData.size(); i += 4)
	{
		CefRefPtr<CefListValue> rect = CefListValue::Create();
		for (int j = 0; j < 4; j++)
		{
			CefRefPtr<CefValue> val = CefValue::Create();
			val->SetDouble(rectData[i + j]);
			rect->SetValue(j, val);
		}

		extracted_data->SetList(i / 4, rect);
	}
	wrapper->SetList(0, extracted_data);
	return wrapper;
}

const CefRefPtr<CefListValue> StringToCefListValue::ListOfStrings(std::string attrData)
{
	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	CefRefPtr<CefListValue> extracted_data = CefListValue::Create();

	std::vector<std::string> options = SplitBySeparator(attrData, ';');
	for (int i = 0; i < (int)options.size(); i++)
	{
		extracted_data->SetString(i, options[i]);
	}
	wrapper->SetList(0, extracted_data);
	return wrapper;
}

const CefRefPtr<CefListValue> StringToCefListValue::ListOfIntegers(std::string attrData)
{
	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	CefRefPtr<CefListValue> extracted_data = CefListValue::Create();

	std::vector<std::string> options = SplitBySeparator(attrData, ';');
	for (int i = 0; i < (int)options.size(); i++)
	{
		extracted_data->SetInt(i, std::stoi(options[i]));
	}
	wrapper->SetList(0, extracted_data);
	return wrapper;
}

const CefRefPtr<CefListValue> StringToCefListValue::ListOfBools(std::string attrData)
{
	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	CefRefPtr<CefListValue> extracted_data = CefListValue::Create();

	std::vector<std::string> options = SplitBySeparator(attrData, ';');
	for (int i = 0; i < (int)options.size(); i++)
	{
		extracted_data->SetBool(i, (std::stoi(options[i]) > 0) );
	}
	wrapper->SetList(0, extracted_data);
	return wrapper;
}

const CefRefPtr<CefListValue> StringToCefListValue::Bitmask(std::string attrData)
{
	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	CefRefPtr<CefListValue> extracted_data = CefListValue::Create();

	for (size_t i = 0; i < attrData.length(); i++)
	{
		extracted_data->SetBool(i, (attrData[i] == '1'));
	}
	wrapper->SetList(0, extracted_data);
	return wrapper;
}

const CefRefPtr<CefListValue> StringToCefListValue::Boolean(std::string attrData)
{
	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	wrapper->SetBool(0, std::stoi(attrData) != 0);
	return wrapper;
}

const CefRefPtr<CefListValue> StringToCefListValue::Integer(std::string attrData)
{
	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	wrapper->SetInt(0, std::stoi(attrData));
	return wrapper;
}

const CefRefPtr<CefListValue> StringToCefListValue::String(std::string attrData)
{
	CefRefPtr<CefListValue> wrapper = CefListValue::Create();
	wrapper->SetString(0, attrData);
	return wrapper;
}

const CefRefPtr<CefListValue> StringToCefListValue::ExtractAttributeData(DOMAttribute attr, std::string attrData)
{
	// Early exit
	if (AttrConversion.find(attr) == AttrConversion.end())
	{
		LogError("StringToCefListValue conversion: Insufficient information defined for DOMAttribute ", (int)attr, "! Abort.");
		return CefRefPtr<CefListValue>();
	}

	return AttrConversion.at(attr)(attrData);
}

