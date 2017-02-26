//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Class for DOM nodes which are used by Tab.

#ifndef DOMNODE_H_
#define DOMNODE_H_

#include "src/Typedefs.h"
#include "src/CEF/Data/Rect.h"
#include "src/Utils/glmWrapper.h"
#include "src/CEF/Data/DOMNodeType.h"
#include <vector>
#include <string>
#include <memory>

// TODO(Daniel): Remove FrameID everywhere.

// General DOM node
class DOMNode
{
public:

	DOMNodeType GetType() const { return _type; } // TODO: Get rid of type, maybe virtual GetType method for every inheriting class 
												  // with constant return value
												  // BUT: type needed for GetDOMNode in Tab!
	int64 GetFrameID() const { return _frameID; }
	int GetNodeID() const { return _nodeId; };
	std::vector<Rect> GetRects() const { return _rects; };
	glm::vec2 GetCenter() const; // unified center of all bounding rects
	std::vector<glm::vec2> GetCenters() const;
	bool GetFixed() const { return _fixed; }
	bool GetVisibility() const { return _visible; }
	const std::string GetText() const { return _text; }
	bool IsPasswordField() const { return _isPasswordField; }
	void AddRect(Rect rect) { _rects.push_back(rect); }
	void SetRects(std::shared_ptr<std::vector<Rect>> rects);
	void SetFixed(bool fixed) { _fixed = fixed; }
	void SetVisibility(bool visible) { _visible = visible; }
	void SetText(std::string text) { _text = text; }
	void SetAsPasswordField() { _isPasswordField = true; }

	// TODO(Daniel): Make DOMNode class purely abstract, get rid of type attribute
	// blank constructor
	DOMNode(DOMNodeType type, int nodeId)
	{
		_nodeId = nodeId;
		_type = type;
	}

	// Constructor for nodes with single bounding rect
	DOMNode(DOMNodeType type, int64 frameID, int nodeID, Rect rect)
	{
		_type = type;
		_frameID = frameID;
		_nodeId = nodeID;
		_rects = { rect };
	}

	// Constructor for node with multiple bounding rects
	DOMNode(DOMNodeType type, int64 frameID, int nodeID, std::vector<Rect> rects)
	{
		_type = type;
		_frameID = frameID;
		_nodeId = nodeID;
		_rects = rects;
	}

protected:

	// Members
	DOMNodeType _type;
	int64 _frameID;
	int _nodeId; // Node's position in JavaScript's list of nodes of the same type
	std::vector<Rect> _rects = { };
	bool _fixed = false;
	bool _visible = true;
	std::string _text = "";
	bool _isPasswordField = false;
};

// DOM node of text input field
class DOMTextInput : public DOMNode
{
public:

	// Constructor
	DOMTextInput(
		DOMNodeType type,
		int64 frameID,
		int nodeID,
		Rect rect,
		std::string value = "");

	std::string GetValue() const { return _value; }

private:

	// Members
	std::string _value = "";
};

// DOM node of link
class DOMLink : public DOMNode
{
public:

	// Constructor for single bounding rect
	DOMLink(
		DOMNodeType type,
		int64 frameID,
		int nodeID,
		Rect rect,
		std::string url);

	// Constructor for multiple bounding rects
	DOMLink(
		DOMNodeType type,
		int64 frameID,
		int nodeID,
		std::vector<Rect> rects,
		std::string url);

	// Getter of URL in link
	std::string GetURL() const { return _url; }

private:

	// Members
	std::string _url;
};

class DOMSelectField : public DOMNode
{
public:
	DOMSelectField(		// TODO: This constructor won't be used?
		int nodeId,
		std::vector<Rect> rects,
		std::vector<std::string> options) : DOMNode(DOMNodeType::SelectField, -1, nodeId, rects)
	{
		_options = options;
	}

	DOMSelectField(int id) : DOMNode(DOMNodeType::SelectField, id) {};

	std::vector<std::string> GetOptions() const { return _options; }
	void SetOptions(std::vector<std::string> options) { _options = options; }

private:
	std::vector<std::string> _options = {}; // Entries might be NULL, if weirdly indexed on JS side (but chances are really low)
};

// Overflow element
class OverflowElement
{
public:

	// Constructor
	OverflowElement(int id, Rect rect, int leftMax, int topMax)
	{
		_id = id;
		_rects.push_back(rect);
		_scrollLeftMax = leftMax;
		_scrollTopMax = topMax;
	}

	// Methods
	int GetId() const { return _id; };
	std::vector<Rect> GetRects() const { return _rects; };
	bool GetFixed() const { return _fixed; };
	std::pair<int, int> GetMaxScrolling() const { return std::make_pair(_scrollLeftMax, _scrollTopMax); }
	std::pair<int, int> GetCurrentScrolling() const { return std::make_pair(_scrollLeft, _scrollTop); }
	void UpdateRect(int rectId, std::shared_ptr<Rect> rect);
	void UpdateFixation(int fixed);

private:

	// Members
	int _id;
	std::vector<Rect> _rects;
	bool _fixed = false;
	int _scrollLeftMax, _scrollTopMax; // maximum values for scrolling directions
	int _scrollLeft = 0, _scrollTop = 0; // current position in interval [0, max value]
};

#endif  // DOMNODE_H_


