//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "DOMAttribute.h"

// Helper for debug output
std::string DOMAttrToString(DOMAttribute attr)
{
	switch (attr) {
	case Rects:				return "Rects";
	case FixedId:			return "FixedId"; 
	case OverflowId:		return "OverflowId"; 
	case Text:				return "Text"; 
	case IsPassword:		return "IsPassword"; 
	case Url:				return "Url"; 
	case Options:			return "Options"; 
	case MaxScrolling:		return "MaxScrolling"; 
	case CurrentScrolling:	return "CurrentScrolling"; 
	case OccBitmask:		return "OccBitmask"; 
	case HTMLId:			return "HTMLId"; 
	case HTMLClass:			return "HTMLClass";
	case CheckedState:		return "CheckedState";
	default:				return std::to_string(attr);
	}
}

DOMAttribute IntToDOMAttribute(int const& val)
{
	return static_cast<DOMAttribute>(val);
}