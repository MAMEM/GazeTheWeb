//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "DOMNode.h"
#include "src/Utils/Logger.h"
#include "src/Utils/Helper.h"
#include <algorithm>
#include <sstream>

/*
    ____  ____  __  ____   __          __   
   / __ \/ __ \/  |/  / | / /___  ____/ /__ 
  / / / / / / / /|_/ /  |/ / __ \/ __  / _ \
 / /_/ / /_/ / /  / / /|  / /_/ / /_/ /  __/
/_____/\____/_/  /_/_/ |_/\____/\__,_/\___/ 
                   
*/
// TODO: Move descriptions to DOMAttribute.cpp?
const std::vector<DOMAttribute> DOMNode::_description = {
	Rects, FixedId, OverflowId, OccBitmask
};

int DOMNode::Initialize(CefRefPtr<CefProcessMessage> msg)
{
	const auto args = msg->GetArgumentList();
	// Check if there are enough arguments in msg to initialize each attribute
	if ((int) _description.size() > args->GetSize() - 1)
	{
		LogError("DOMNode: On initialization: Object description (", _description.size(), ") and message (", args->GetSize(), 
			") size do not match! id: ",_id);
	}
	else
	{
		for (unsigned int i = 0; i < _description.size(); i++)
		{
			CefRefPtr<CefListValue> data = args->GetList(i + 1);	
			if (
				!Update(_description[i], data)
				)
			{
				LogError("DOMNode: Failed to assign value of type ", data->GetType(0),
					" to attribute ", DOMAttrToString(_description[i]), "!");
			}
		}
	}
	return _description.size() + 1;
}

bool DOMNode::Update(DOMAttribute attr, CefRefPtr<CefListValue> data)
{	
	switch (attr) {
		case DOMAttribute::Rects:			return IPCSetRects(data);
		case DOMAttribute::FixedId:			return IPCSetFixedId(data);
		case DOMAttribute::OverflowId:		return IPCSetOverflowId(data);
		case DOMAttribute::OccBitmask:		return IPCSetOccBitmask(data);
	}

	LogError("DOMNode: Could not find attribute ", attr, " in order to assign data");
	return false;
}

bool DOMNode::PrintAttribute(DOMAttribute attr)
{
	if (std::find(_description.begin(), _description.end(), attr) == _description.end())
	{
		LogInfo("DOMAttrReq: DOMAttribute '", DOMAttrToString(attr), "' could not been found!");
		return false;
	}
	
	std::ostringstream acc;
	acc << "DOMAttrReq: " << DOMAttrToString(attr) << std::endl;
	switch (attr) {
	case Rects: { 
		int count = 0;
		for (auto rect : GetRects())
			acc << "\t" << std::to_string(count++) << ": " << rect.ToString() << std::endl;
		break;
	}
	case FixedId: { acc << "\t" << std::to_string(GetFixedId()) << std::endl; break; }
	case OverflowId: { acc << "\t" << std::to_string(GetOverflowId()) << std::endl; break; }
	case OccBitmask: {
		acc << "\t";
		for (const auto& bit : _occBitmask)
			acc << std::to_string(static_cast<int>(bit));
		acc << std::endl;
		break;
	}
	}
	LogInfo(acc.str());
	return true;
}

bool DOMNode::IPCSetRects(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() > 0  && data->GetValue(0)->GetType() != CefValueType::VTYPE_LIST)
		return false;

	const auto rectList = data->GetList(0);
	std::vector<Rect> rects;

	for (int i = 0; i < (int)rectList->GetSize(); i++)
	{
		const auto rectData = rectList->GetList(i);
		std::vector<float> rect;
		for (int j = 0; rectData && j < (int)rectData->GetSize(); j++)
		{
			rect.push_back(rectData->GetValue(j)->GetDouble());
		}
		rects.push_back(Rect(rect));
	}

	SetRects(rects);
	return true;
}

bool DOMNode::IPCSetFixedId(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1 || data->GetType(0) != CefValueType::VTYPE_INT)
	{
		LogDebug("Expected CefValueType: ", CefValueType::VTYPE_INT, ", but got: ", data->GetType(0));
		return false;
	}

	SetFixedId(data->GetInt(0));
	return true;
}

bool DOMNode::IPCSetOverflowId(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1 || data->GetType(0) != CefValueType::VTYPE_INT)
		return false;

	SetOverflowId(data->GetInt(0));
	return true;
}

bool DOMNode::IPCSetOccBitmask(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() > 0 && data->GetValue(0)->GetType() != CefValueType::VTYPE_LIST)
		return false;

	const auto list = data->GetList(0);
	std::vector<bool> mask;

	for (int i = 0; i < (int)list->GetSize(); i++)
	{
		mask.push_back(list->GetBool(i));
	}

	SetOccBitmask(mask);
	return true;
}

/*
    ____  ____  __  _________          __  ____                  __ 
   / __ \/ __ \/  |/  /_  __/__  _  __/ /_/  _/___  ____  __  __/ /_
  / / / / / / / /|_/ / / / / _ \| |/_/ __// // __ \/ __ \/ / / / __/
 / /_/ / /_/ / /  / / / / /  __/>  </ /__/ // / / / /_/ / /_/ / /_  
/_____/\____/_/  /_/ /_/  \___/_/|_|\__/___/_/ /_/ .___/\__,_/\__/  
                                                /_/                 
*/
const std::vector<DOMAttribute> DOMTextInput::_description = {
	Text, IsPassword, HTMLId, HTMLClass
};

int DOMTextInput::Initialize(CefRefPtr<CefProcessMessage> msg)
{
	// First list element to start interpretation as this class's attributes
	int pivot = super::Initialize(msg);

	const auto args = msg->GetArgumentList();
	// Check if there are enough arguments in msg to initialize each attribute
	if ((int)_description.size() > (int) args->GetSize() - pivot)
	{
		LogError("DOMTextInput: On initialization: Object description and message size do not match!");
	}
	else
	{
		for (unsigned int i = 0; i < _description.size(); i++)
		{
			CefRefPtr<CefListValue> data = args->GetList(pivot + i);

			if (!Update(_description[i], data))
			{
				LogError("DOMTextInput: Failed to assign value of type ", args->GetValue(i + 1)->GetType(),
					" to attribute ", DOMAttrToString(_description[i]), "!");
			}
		}
	}
	return _description.size() + pivot;
}

bool DOMTextInput::Update(DOMAttribute attr, CefRefPtr<CefListValue> data)
{
	switch (attr) {
		case DOMAttribute::Text:			return IPCSetText(data);
		case DOMAttribute::IsPassword:		return IPCSetPassword(data);
		case DOMAttribute::HTMLId:			return IPCSetHTMLId(data);
		case DOMAttribute::HTMLClass:		return IPCSetHTMLClass(data);
	}
	return super::Update(attr, data);
}


bool DOMTextInput::PrintAttribute(DOMAttribute attr)
{
	if (std::find(_description.begin(), _description.end(), attr) == _description.end())
	{
		return super::PrintAttribute(attr);
	}

	std::ostringstream acc;
	acc << "DOMAttrReq: " << DOMAttrToString(attr) << std::endl;
	switch (attr) {
	case Text: { acc << "\t" << GetText() << std::endl; break; }
	case IsPassword: { acc << "\t" << std::to_string(IsPasswordField()) << std::endl; break; }
	}
	LogInfo(acc.str());
	return true;
}

bool DOMTextInput::IPCSetText(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1 || data->GetValue(0)->GetType() != CefValueType::VTYPE_STRING)
		return false;

	SetText(data->GetString(0).ToString());
	return true;
}

bool DOMTextInput::IPCSetPassword(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1 || data->GetValue(0)->GetType() != CefValueType::VTYPE_BOOL)
		return false;

	SetPassword(data->GetBool(0));
	return true;
}

bool DOMTextInput::IPCSetHTMLId(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1 || data->GetValue(0)->GetType() != CefValueType::VTYPE_STRING)
		return false;

	SetHTMLId(data->GetString(0).ToString());
	return true;
}

bool DOMTextInput::IPCSetHTMLClass(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1 || data->GetValue(0)->GetType() != CefValueType::VTYPE_STRING)
		return false;

	SetHTMLClass(data->GetString(0).ToString());
	return true;
}

/*
    ____  ____  __  _____    _       __  
   / __ \/ __ \/  |/  / /   (_)___  / /__
  / / / / / / / /|_/ / /   / / __ \/ //_/
 / /_/ / /_/ / /  / / /___/ / / / / ,<   
/_____/\____/_/  /_/_____/_/_/ /_/_/|_|  
 
*/
const std::vector<DOMAttribute> DOMLink::_description = {
	Text, Url
};

int DOMLink::Initialize(CefRefPtr<CefProcessMessage> msg)
{
	// First list element to start interpretation as this class's attributes
	int pivot = super::Initialize(msg);

	const auto args = msg->GetArgumentList();
	// Check if there are enough arguments in msg to initialize each attribute
	if ((int)_description.size() > (int) args->GetSize() - pivot)
	{
		LogError("DOMLink: On initialization: Object description ", _description.size(), " + " , pivot, " and message size ", args->GetSize() ," do not match!");
	}
	else
	{
		for (unsigned int i = 0; i < _description.size(); i++)
		{
			CefRefPtr<CefListValue> data = args->GetList(pivot + i);	// Access argument list, where super class has finished

			if (!Update(_description[i], data))
			{
				LogError("DOMLink: Failed to assign value of type ", data->GetValue(0)->GetType(), // TODO: Could this crash the renderer process if data is a CefListValue?
					" to attribute ", DOMAttrToString(_description[i]), "!");
			}
		}
	}
	return _description.size() + pivot;
}

bool DOMLink::Update(DOMAttribute attr, CefRefPtr<CefListValue> data)
{
	switch (attr) {
		case DOMAttribute::Text:	return IPCSetText(data);
		case DOMAttribute::Url:		return IPCSetUrl(data);
	}

	return super::Update(attr, data);
}

bool DOMLink::PrintAttribute(DOMAttribute attr)
{
	if (std::find(_description.begin(), _description.end(), attr) == _description.end())
	{
		return super::PrintAttribute(attr);
	}

	std::ostringstream acc;
	acc << "DOMAttrReq: " << DOMAttrToString(attr) << std::endl;
	switch (attr) {
	case Text: { acc << "\t" << GetText() << std::endl; break; }
	case Url: { acc << "\t" << GetUrl() << std::endl; break; }
	}
	LogInfo(acc.str());
	return true;
}

bool DOMLink::IPCSetText(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1 || data->GetValue(0)->GetType() != CefValueType::VTYPE_STRING)
		return false;

	SetText(data->GetString(0).ToString());
	return true;
}

bool DOMLink::IPCSetUrl(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1 || data->GetValue(0)->GetType() != CefValueType::VTYPE_STRING)
		return false;

	SetUrl(data->GetString(0).ToString());
	return true;
}

/*
    ____  ____  __  ________      __          __  _______      __    __
   / __ \/ __ \/  |/  / ___/___  / /__  _____/ /_/ ____(_)__  / /___/ /
  / / / / / / / /|_/ /\__ \/ _ \/ / _ \/ ___/ __/ /_  / / _ \/ / __  / 
 / /_/ / /_/ / /  / /___/ /  __/ /  __/ /__/ /_/ __/ / /  __/ / /_/ /  
/_____/\____/_/  /_//____/\___/_/\___/\___/\__/_/   /_/\___/_/\__,_/   
                                                                     
*/

const std::vector<DOMAttribute> DOMSelectField::_description = {
	Options
};

int DOMSelectField::Initialize(CefRefPtr<CefProcessMessage> msg)
{
	// First list element to start interpretation as this class's attributes
	int pivot = super::Initialize(msg);

	const auto args = msg->GetArgumentList();
	// Check if there are enough arguments in msg to initialize each attribute
	if ((int)_description.size() > (int) args->GetSize() - pivot)
	{
		LogError("DOMSelectField: On initialization: Object description and message size do not match!");
	}
	else
	{
		for (unsigned int i = 0; i < _description.size(); i++)
		{
			CefRefPtr<CefListValue> data = args->GetList(pivot + i);

			if (!Update(_description[i], data))
			{
				LogError("DOMSelectField: Failed to assign value of type ", args->GetValue(i + 1)->GetType(),
					" to attribute ", DOMAttrToString(_description[i]), "!");
			}
		}
	}
	return _description.size() + pivot;
}

bool DOMSelectField::Update(DOMAttribute attr, CefRefPtr<CefListValue> data)
{
	switch (attr) {
		case DOMAttribute::Options:		return IPCSetOptions(data);
	}
	return super::Update(attr, data);
}

bool DOMSelectField::PrintAttribute(DOMAttribute attr)
{
	if (std::find(_description.begin(), _description.end(), attr) == _description.end())
	{
		return super::PrintAttribute(attr);
	}

	std::ostringstream acc;
	acc << "DOMAttrReq: " << DOMAttrToString(attr) << std::endl;
	switch (attr) {
	case Options: {
		int count = 0;
		for (auto opt : GetOptions())
			acc << "\t" << std::to_string(count++) << ": " << opt << std::endl;
		break;
	}
	}
	LogInfo(acc.str());
	return true;
}

bool DOMSelectField::IPCSetOptions(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1)
	{
		LogDebug("DOMSelectField: Size of option list is less than 1. Aborting.");
		return false;
	}

	std::vector<std::string> options;
	CefRefPtr<CefListValue> list = data->GetList(0);
	for (int i = 0; i < (int)list->GetSize(); i++)
	{
		if (list->GetType(i) != CefValueType::VTYPE_STRING)
		{
			LogDebug("DOMSelectField: Invalid data type for option! Aborting.");
			return false;
		}
		options.push_back(list->GetString(i).ToString());
	}

	SetOptions(options);
	return true;
}

/*
    ____  ____  __  _______                  ______              ________                          __ 
   / __ \/ __ \/  |/  / __ \_   _____  _____/ __/ /___ _      __/ ____/ /__  ____ ___  ___  ____  / /_
  / / / / / / / /|_/ / / / / | / / _ \/ ___/ /_/ / __ \ | /| / / __/ / / _ \/ __ `__ \/ _ \/ __ \/ __/
 / /_/ / /_/ / /  / / /_/ /| |/ /  __/ /  / __/ / /_/ / |/ |/ / /___/ /  __/ / / / / /  __/ / / / /_  
/_____/\____/_/  /_/\____/ |___/\___/_/  /_/ /_/\____/|__/|__/_____/_/\___/_/ /_/ /_/\___/_/ /_/\__/  
                                                                                                    
*/

const std::vector<DOMAttribute> DOMOverflowElement::_description = {
	MaxScrolling, CurrentScrolling
};

int DOMOverflowElement::Initialize(CefRefPtr<CefProcessMessage> msg)
{

	// First list element to start interpretation as this class's attributes
	int pivot = super::Initialize(msg);

	const auto args = msg->GetArgumentList();
	// Check if there are enough arguments in msg to initialize each attribute
	if ((int)_description.size() > (int) args->GetSize() - pivot)
	{
		LogError("OverflowElement: On initialization: Object description and message size do not match!");
	}
	else
	{
		for (unsigned int i = 0; i < _description.size(); i++)
		{
			CefRefPtr<CefListValue> data = args->GetList(pivot + i);

			if (!Update(_description[i], data))
			{
				LogError("OverflowElement: Failed to assign value of type ", data->GetType(0),
					" to attribute ", DOMAttrToString(_description[i]), "!");
			}
		}
	}


	return _description.size() + pivot;
}

bool DOMOverflowElement::Update(DOMAttribute attr, CefRefPtr<CefListValue> data)
{
	switch (attr) {
	case DOMAttribute::MaxScrolling:		return IPCSetMaxScrolling(data);
	case DOMAttribute::CurrentScrolling:	return IPCSetCurrentScrolling(data);
	}
	return super::Update(attr, data);
}

bool DOMOverflowElement::PrintAttribute(DOMAttribute attr)
{
	if (std::find(_description.begin(), _description.end(), attr) == _description.end())
	{
		return super::PrintAttribute(attr);
	}

	std::ostringstream acc;
	acc << "DOMAttrReq: " << DOMAttrToString(attr) << std::endl;
	switch (attr) {
	case MaxScrolling: {
		acc << "\t" << std::to_string(GetMaxScrolling().first) << ", " <<
			std::to_string(GetMaxScrolling().second) << std::endl;
		break;
	}
	case CurrentScrolling: {
		acc << "\t" << std::to_string(GetCurrentScrolling().first) << ", " <<
			std::to_string(GetCurrentScrolling().second) << std::endl;
		break;
	}
	}
	LogInfo(acc.str());
	return true;
}

bool DOMOverflowElement::IPCSetMaxScrolling(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1)
		return false;

	CefRefPtr<CefListValue> list = data->GetList(0);
	if (list->GetSize() < 2 || list->GetType(0) != CefValueType::VTYPE_INT || list->GetType(1) != CefValueType::VTYPE_INT)
		return false;

	SetMaxScrolling(list->GetInt(0), list->GetInt(1));
	return true;
}

bool DOMOverflowElement::IPCSetCurrentScrolling(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1)
		return false;

	CefRefPtr<CefListValue> list = data->GetList(0);
	if (list->GetSize() < 2 || list->GetType(0) != CefValueType::VTYPE_INT || list->GetType(1) != CefValueType::VTYPE_INT)
		return false;

	SetCurrentScrolling(list->GetInt(0), list->GetInt(1));
	return true;
}

// TODO: Get rid of this somehow...
void DOM::GetJSRepresentation(
	std::string nodeType,
	std::vector<const std::vector<DOMAttribute>*>& description,
	std::string & obj_getter_name)
{
	if (nodeType == "TextInputData")
	{
		DOMTextInput::GetDescription(&description);
		obj_getter_name = DOMTextInput::GetJSObjectGetter();
		return;

	}
	if (nodeType == "LinkData")
	{
		DOMLink::GetDescription(&description);
		obj_getter_name = DOMLink::GetJSObjectGetter();
		return;

	}
	if (nodeType == "SelectFieldData")
	{
		DOMSelectField::GetDescription(&description);
		obj_getter_name = DOMSelectField::GetJSObjectGetter();
		return;

	}
	if (nodeType == "OverflowElementData")
	{
		DOMOverflowElement::GetDescription(&description);
		obj_getter_name = DOMOverflowElement::GetJSObjectGetter(); 
		return;
	}
	if (nodeType == "VideoData")
	{
		DOMVideo::GetDescription(&description);
		obj_getter_name = DOMVideo::GetJSObjectGetter();
		return;
	}
	if (nodeType == "CheckboxData")
	{
		DOMCheckbox::GetDescription(&description);
		obj_getter_name = DOMCheckbox::GetJSObjectGetter();
		return;
	}
}

/*
    ____  ____  __  ____    ___     __         
   / __ \/ __ \/  |/  / |  / (_)___/ /__  ____ 
  / / / / / / / /|_/ /| | / / / __  / _ \/ __ \
 / /_/ / /_/ / /  / / | |/ / / /_/ /  __/ /_/ /
/_____/\____/_/  /_/  |___/_/\__,_/\___/\____/ 
*/

const std::vector<DOMAttribute> DOMVideo::_description = { };

int DOMVideo::Initialize(CefRefPtr<CefProcessMessage> msg)
{

	// First list element to start interpretation as this class's attributes
	int pivot = super::Initialize(msg);

	const auto args = msg->GetArgumentList();
	// Check if there are enough arguments in msg to initialize each attribute
	if ((int)_description.size() > (int)args->GetSize() - pivot)
	{
		LogError("DOMVideo: On initialization: Object description and message size do not match!");
	}
	else
	{
		for (unsigned int i = 0; i < _description.size(); i++)
		{
			CefRefPtr<CefListValue> data = args->GetList(pivot + i);

			if (!Update(_description[i], data))
			{
				LogError("DOMVideo: Failed to assign value of type ", data->GetType(0),
					" to attribute ", DOMAttrToString(_description[i]), "!");
			}
		}
	}


	return _description.size() + pivot;
}

bool DOMVideo::Update(DOMAttribute attr, CefRefPtr<CefListValue> data)
{
	//switch (attr) {
	//}
	return super::Update(attr, data);
}

bool DOMVideo::PrintAttribute(DOMAttribute attr)
{
	return super::PrintAttribute(attr);
}

/*
    ____  ____  __  ___________              __   __              
   / __ \/ __ \/  |/  / ____/ /_  ___  _____/ /__/ /_  ____  _  __
  / / / / / / / /|_/ / /   / __ \/ _ \/ ___/ //_/ __ \/ __ \| |/_/
 / /_/ / /_/ / /  / / /___/ / / /  __/ /__/ ,< / /_/ / /_/ />  <  
/_____/\____/_/  /_/\____/_/ /_/\___/\___/_/|_/_.___/\____/_/|_|  
 
*/

const std::vector<DOMAttribute> DOMCheckbox::_description = { CheckedState };

int DOMCheckbox::Initialize(CefRefPtr<CefProcessMessage> msg)
{
	// First list element to start interpretation as this class's attributes
	int pivot = super::Initialize(msg);

	const auto args = msg->GetArgumentList();
	// Check if there are enough arguments in msg to initialize each attribute
	if ((int)_description.size() > (int)args->GetSize() - pivot)
	{
		LogError("DOMCheckbox: On initialization: Object description and message size do not match!");
	}
	else
	{
		for (unsigned int i = 0; i < _description.size(); i++)
		{
			CefRefPtr<CefListValue> data = args->GetList(pivot + i);

			if (!Update(_description[i], data))
			{
				LogError("DOMCheckbox: Failed to assign value of type ", data->GetType(0),
					" to attribute ", DOMAttrToString(_description[i]), "!");
			}
		}
	}


	return _description.size() + pivot;
}

bool DOMCheckbox::Update(DOMAttribute attr, CefRefPtr<CefListValue> data)
{
	switch (attr) {
	case CheckedState: { return IPCSetCheckedState(data); }
	}
	return super::Update(attr, data);
}

bool DOMCheckbox::IPCSetCheckedState(CefRefPtr<CefListValue> data)
{
	if (data == nullptr || data->GetSize() < 1 || data->GetValue(0)->GetType() != CefValueType::VTYPE_BOOL)
		return false;

	SetCheckedState(data->GetBool(0));
	return true;
}

bool DOMCheckbox::PrintAttribute(DOMAttribute attr)
{
	if (std::find(_description.begin(), _description.end(), attr) == _description.end())
	{
		return super::PrintAttribute(attr);
	}

	std::ostringstream acc;
	acc << "DOMAttrReq: " << DOMAttrToString(attr) << std::endl;
	switch (attr) {
	case CheckedState: {
		acc << "\t" << GetCheckedState() << std::endl;
		break;
	}
	}
	LogInfo(acc.str());
	return true;
}