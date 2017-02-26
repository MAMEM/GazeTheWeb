//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "DOMNode.h"
#include "src/Utils/Logger.h"

/* DOMNode methods*/

glm::vec2 DOMNode::GetCenter() const
{
	glm::vec2 center(0, 0);
	if (_rects.size() > 0)
	{
		for (int i = 0; i < (int)_rects.size(); i++)
		{
			center += _rects[i].Center();
		}
		center /= _rects.size();
	}
	return center;
}

std::vector<glm::vec2> DOMNode::GetCenters() const
{
	std::vector<glm::vec2> centers;
	for (int i = 0; i < (int)_rects.size(); i++)
	{
		centers.push_back(_rects[i].Center());
	}
	return centers;
}

void DOMNode::SetRects(std::shared_ptr<std::vector<Rect>> rects)
{
	_rects = *rects.get();
}

/* DOMTextInput methods*/

DOMTextInput::DOMTextInput(DOMNodeType type,
	int64 frameID,
	int nodeId,
	Rect rect,
	std::string value) : DOMNode(type, frameID, nodeId, rect)
{
	_value = value;
}

/* DOMLink methods */

DOMLink::DOMLink(DOMNodeType type,
	int64 frameID,
	int nodeId,
	Rect rect,
	std::string url) : DOMNode(type, frameID, nodeId, rect)
{
	_url = url;
}

DOMLink::DOMLink(
	DOMNodeType type,
	int64 frameID,
	int nodeId,
	std::vector<Rect> rects,
	std::string url) : DOMNode(type, frameID, nodeId, rects)
{
	_url = url;
}

/* Overflow methods */

void OverflowElement::UpdateRect(int rectId, std::shared_ptr<Rect> rect)
{
	if (rectId < _rects.size() && rectId >= 0)
	{
		_rects[rectId] = *rect.get();
	}
	else
	{
		LogBug("OverflowElement: Tried to update ", rectId, "th Rect, but it doesn't exist.");
	}
}

void OverflowElement::UpdateFixation(int fixed)
{
	_fixed = fixed;
}