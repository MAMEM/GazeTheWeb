//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Class for DOM nodes which are used by Tab.

#ifndef DOMNODE_H_
#define DOMNODE_H_

#pragma warning(push) // warning about dominance inheritage
#pragma warning(disable : 4250)

#include "src/CEF/Data/DOMNodeInteraction.h"
#include "src/CEF/Data/Rect.h"
#include "src/Utils/glmWrapper.h"
#include "src/CEF/Data/DOMAttribute.h"
#include "include/cef_process_message.h"
#include <vector>
#include <string>
#include <memory>
#include "src/Utils/Logger.h" // DEBUGGING

namespace DOM
{
	void GetJSRepresentation(
		std::string nodeType,
		std::vector<const std::vector<DOMAttribute>* >& description,
		std::string& obj_getter_name
		);
}

/*
    ____  ____  __  ____   __          __   
   / __ \/ __ \/  |/  / | / /___  ____/ /__ 
  / / / / / / / /|_/ /  |/ / __ \/ __  / _ \
 / /_/ / /_/ / /  / / /|  / /_/ / /_/ /  __/
/_____/\____/_/  /_/_/ |_/\____/\__,_/\___/ 
         
*/
class DOMNode :
	public virtual DOMBaseInterface
{
public:

	// Empty construction
	DOMNode(int id) :
        _id(id) {}

	// Define initialization through IPC message in each DOMNode subclass
	virtual int Initialize(CefRefPtr<CefProcessMessage> msg);

	// CefProcessMessage to C++ object. So update indicated by JavaScript
	virtual bool Update(DOMAttribute attr, CefRefPtr<CefListValue> data);

	// Contains all
	static void GetDescription(std::vector<const std::vector<DOMAttribute>* >* descriptions) {
		descriptions->push_back(&_description);
	}

	// Enable accessing attribute data while running the program
	virtual bool PrintAttribute(DOMAttribute attr);	// TODO: Return or receive genericly typed value and check if same value is on Javascript side?
	
	// Getter from DOMBaseInterface
	virtual int GetId() override { return _id; }

	// Custom final getter
	std::vector<Rect> GetRects() const { return _rects; }
	int GetFixedId() const { return _fixedId; }
	int GetOverflowId() const { return _overflowId; }
	bool IsFixed() const { return (_fixedId >= 0); }
	bool IsOccluded() const { return _occluded; }

private:

	// Setter
	void SetId(int id) { _id = id; }
	void SetRects(std::vector<Rect> rects) { _rects = rects; }
	void SetFixedId(int fixedId) { _fixedId = fixedId; }
	void SetOverflowId(int overflowId) { _overflowId = overflowId; }
	void SetOccBitmask(std::vector<bool> bitmask) { 
		_occBitmask = bitmask; 
		_occluded = true; 
		for (const auto rOcc : _occBitmask) 
		{ 
			_occluded &= rOcc;
		}
	}

	bool IPCSetRects(CefRefPtr<CefListValue> data);
	bool IPCSetFixedId(CefRefPtr<CefListValue> data);
	bool IPCSetOverflowId(CefRefPtr<CefListValue> data);
	bool IPCSetOccBitmask(CefRefPtr<CefListValue> data);

	// Members
	static const std::vector<DOMAttribute> _description;
	int _id;
	std::vector<Rect> _rects = {};
	int _fixedId = -1;		// first FixedElement's ID, which is hierarchically above this node, if any
	int _overflowId = -1;	// first DOMOverflowElement's ID, which is hierarchically above this node, if any
	std::vector<bool> _occBitmask;
	bool _occluded = false; // true if occluded
};

/*
    ____  ____  __  _________          __  ____                  __ 
   / __ \/ __ \/  |/  /_  __/__  _  __/ /_/  _/___  ____  __  __/ /_
  / / / / / / / /|_/ / / / / _ \| |/_/ __// // __ \/ __ \/ / / / __/
 / /_/ / /_/ / /  / / / / /  __/>  </ /__/ // / / / /_/ / /_/ / /_  
/_____/\____/_/  /_/ /_/  \___/_/|_|\__/___/_/ /_/ .___/\__,_/\__/  
                                                /_/                 
*/

class DOMTextInput : 
	public virtual DOMNode,
	public virtual DOMTextInputInteraction
{
public:

	// Empty construction
	DOMTextInput(int id, TabDOMNodeInterface* pTab) :
		DOMNode(id),
        DOMJavascriptCommunication(pTab),
        DOMTextInputInteraction() {}

	// Define initialization through ICP message in each DOMNode subclass
	virtual int Initialize(CefRefPtr<CefProcessMessage> msg) override;

	// CefProcessMessage to C++ object
	virtual bool Update(DOMAttribute attr, CefRefPtr<CefListValue> data) override;

	// Build description
	static void GetDescription(std::vector<const std::vector<DOMAttribute>* >* descriptions) {
		super::GetDescription(descriptions);
		descriptions->push_back(&_description);
	}

	// TODO: why no abstract declaration in DOMNode?
	static const std::string GetJSObjectGetter()
	{
		return "GetDOMTextInput";
	}

	virtual bool PrintAttribute(DOMAttribute attr);

	// TODO: somehow set enum per subclass and return this in DOMNode?
    virtual int GetType() override { return 0; }
	
	// Custom getter
	std::string GetText() const { return _text; }
	bool IsPasswordField() const { return _isPassword; }
	std::string GetHTMLId() const { return _htmlId; }
	std::string GetHTMLClass() const { return _htmlClass; }

private:

	typedef DOMNode super;

	// Setter
	void SetText(std::string text) { _text = text; }
	void SetPassword(bool isPwd) { _isPassword = isPwd; }
	void SetHTMLId(std::string htmlId) { _htmlId = htmlId; }
	void SetHTMLClass(std::string htmlClass) { _htmlClass = htmlClass; }

	bool IPCSetText(CefRefPtr<CefListValue> data);
	bool IPCSetPassword(CefRefPtr<CefListValue> data);
	bool IPCSetHTMLId(CefRefPtr<CefListValue> data);
	bool IPCSetHTMLClass(CefRefPtr<CefListValue> data);


	// Members
	static const std::vector<DOMAttribute> _description;
	std::string _text = "";
	bool _isPassword = false;
	std::string _htmlId = "";
	std::string _htmlClass = "";

};

/*
    ____  ____  __  _____    _       __  
   / __ \/ __ \/  |/  / /   (_)___  / /__
  / / / / / / / /|_/ / /   / / __ \/ //_/
 / /_/ / /_/ / /  / / /___/ / / / / ,<   
/_____/\____/_/  /_/_____/_/_/ /_/_/|_|  
 
*/

class DOMLink :
	public virtual DOMNode

{
public:

	// Empty construction
	DOMLink(int id) :
		DOMNode(id) {};

	// Define initialization through ICP message in each DOMNode subclass
	virtual int Initialize(CefRefPtr<CefProcessMessage> msg) override;

	// CefProcessMessage to C++ object
	virtual bool Update(DOMAttribute attr, CefRefPtr<CefListValue> data) override;

	// Build description
	static void GetDescription(std::vector<const std::vector<DOMAttribute>* >* descriptions) {
		super::GetDescription(descriptions);
		descriptions->push_back(&_description);
	}

	// TODO: why no abstract declaration in DOMNode?
	static const std::string GetJSObjectGetter()
	{
		return "GetDOMLink";
	}

	virtual bool PrintAttribute(DOMAttribute attr);

	// TODO: somehow set enum per subclass and return this in DOMNode?
    virtual int GetType() override { return 1; }

	// Custom getter
	std::string GetText() const { return _text; }
	std::string GetUrl() const { return _url; }

private:

	typedef DOMNode super;

	// Setter
	void SetText(std::string text) { _text = text; }
	void SetUrl(std::string url) { _url = url; }

	bool IPCSetText(CefRefPtr<CefListValue> data);
	bool IPCSetUrl(CefRefPtr<CefListValue> data);

	// Members
	static const std::vector<DOMAttribute> _description;
	std::string _text = "";
	std::string _url = "";
};

/*
    ____  ____  __  ________      __          __  _______      __    __
   / __ \/ __ \/  |/  / ___/___  / /__  _____/ /_/ ____(_)__  / /___/ /
  / / / / / / / /|_/ /\__ \/ _ \/ / _ \/ ___/ __/ /_  / / _ \/ / __  / 
 / /_/ / /_/ / /  / /___/ /  __/ /  __/ /__/ /_/ __/ / /  __/ / /_/ /  
/_____/\____/_/  /_//____/\___/_/\___/\___/\__/_/   /_/\___/_/\__,_/   
                                                                 
*/

class DOMSelectField : 
	public virtual DOMNode,
	public virtual DOMSelectFieldInteraction
{
public:

	// Empty construction
	DOMSelectField(int id, TabDOMNodeInterface* pTab) :
		DOMNode(id), 
        DOMJavascriptCommunication(pTab),
        DOMSelectFieldInteraction() {}

	// Define initialization through ICP message in each DOMNode subclass
	virtual int Initialize(CefRefPtr<CefProcessMessage> msg) override;

	// CefProcessMessage to C++ object
	virtual bool Update(DOMAttribute attr, CefRefPtr<CefListValue> data) override;

	// Build description
	static void GetDescription(std::vector<const std::vector<DOMAttribute>* >* descriptions) {
		super::GetDescription(descriptions);
		descriptions->push_back(&_description);
	}

	// TODO: why no abstract declaration in DOMNode?
	static const std::string GetJSObjectGetter()
	{
		return "GetDOMSelectField";
	}

	virtual bool PrintAttribute(DOMAttribute attr);

	// TODO: somehow set enum per subclass and return this in DOMNode?
    virtual int GetType() override { return 2; }

	// Custom getter
	std::vector<std::string> GetOptions() const { return _options; }

private:

	typedef DOMNode super;

	// Setter
	void SetOptions(std::vector<std::string> options) { _options = options; }

	bool IPCSetOptions(CefRefPtr<CefListValue> data);

	// Members
	static const std::vector<DOMAttribute> _description;
	std::vector<std::string> _options = {}; // Entries might be NULL, if weirdly indexed on JS side (but chances are really low)
};

/*
    ____  ____  __  _______                  ______              ________                          __ 
   / __ \/ __ \/  |/  / __ \_   _____  _____/ __/ /___ _      __/ ____/ /__  ____ ___  ___  ____  / /_
  / / / / / / / /|_/ / / / / | / / _ \/ ___/ /_/ / __ \ | /| / / __/ / / _ \/ __ `__ \/ _ \/ __ \/ __/
 / /_/ / /_/ / /  / / /_/ /| |/ /  __/ /  / __/ / /_/ / |/ |/ / /___/ /  __/ / / / / /  __/ / / / /_  
/_____/\____/_/  /_/\____/ |___/\___/_/  /_/ /_/\____/|__/|__/_____/_/\___/_/ /_/ /_/\___/_/ /_/\__/  
                                                                                                    
*/

class DOMOverflowElement : 
	public virtual DOMNode,
	public virtual DOMOverflowElementInteraction
{
public:

	// Empty construction
	DOMOverflowElement(int id, TabDOMNodeInterface* pTab) :
		DOMNode(id),
        DOMJavascriptCommunication(pTab),
        DOMOverflowElementInteraction() {}

	// Define initialization through ICP message in each DOMNode subclass
	virtual int Initialize(CefRefPtr<CefProcessMessage> msg) override;

	// CefProcessMessage to C++ object
	virtual bool Update(DOMAttribute attr, CefRefPtr<CefListValue> data) override;

	// Build description
	static void GetDescription(std::vector<const std::vector<DOMAttribute>* >* descriptions) {
		super::GetDescription(descriptions);
		descriptions->push_back(&_description);
	}

	// TODO: why no abstract declaration in DOMNode?
	static const std::string GetJSObjectGetter()
	{
		return "GetDOMOverflowElement";
	}

	virtual bool PrintAttribute(DOMAttribute attr);

	// TODO: somehow set enum per subclass and return this in DOMNode?
    virtual int GetType() override { return 3; }

	// Custom getter
	std::pair<int, int> GetMaxScrolling() const { return std::make_pair(_scrollLeftMax, _scrollTopMax); }
	std::pair<int, int> GetCurrentScrolling() const { return std::make_pair(_scrollLeft, _scrollTop); }

private:

	typedef DOMNode super;

	// Setter
	void SetMaxScrolling(int top, int left) { _scrollTopMax = top; _scrollLeftMax = left; }
	void SetCurrentScrolling(int top, int left) { _scrollTop = top; _scrollLeft = left; }
	
	bool IPCSetMaxScrolling(CefRefPtr<CefListValue> data);
	bool IPCSetCurrentScrolling(CefRefPtr<CefListValue> data);

	// Members
	static const std::vector<DOMAttribute> _description;
	int _scrollLeftMax = 0;		// maximum values for scrolling directions
	int _scrollTopMax = 0; 
	int _scrollLeft = 0;		// current position in interval [0, max value]
	int _scrollTop = 0; 
};

/*
    ____  ____  __  ____    ___     __         
   / __ \/ __ \/  |/  / |  / (_)___/ /__  ____ 
  / / / / / / / /|_/ /| | / / / __  / _ \/ __ \
 / /_/ / /_/ / /  / / | |/ / / /_/ /  __/ /_/ /
/_____/\____/_/  /_/  |___/_/\__,_/\___/\____/ 

*/
class DOMVideo :
	public virtual DOMNode,
	public virtual DOMVideoInteraction
{
public:

	// Empty construction
	DOMVideo(int id, TabDOMNodeInterface* pTab) :
		DOMNode(id),
		DOMJavascriptCommunication(pTab),
		DOMVideoInteraction() {}

	// Define initialization through ICP message in each DOMNode subclass
	virtual int Initialize(CefRefPtr<CefProcessMessage> msg) override;

	// Extract data from CefProcessMessage to C++ object's corresponding attibute
	virtual bool Update(DOMAttribute attr, CefRefPtr<CefListValue> data) override;

	// Build description
	static void GetDescription(std::vector< const std::vector<DOMAttribute>* >* descriptions) {
		super::GetDescription(descriptions);
		descriptions->push_back(&_description);
	}

	// TODO: why no abstract declaration in DOMNode?
	static const std::string GetJSObjectGetter()
	{
		return "GetDOMVideo";
	}

	virtual bool PrintAttribute(DOMAttribute attr);

	// TODO: somehow set enum per subclass and return this in DOMNode?
	virtual int GetType() override { return 4; }


private:

	typedef DOMNode super;

	// Setter

	// Members
	static const std::vector<DOMAttribute> _description;
};


/*
    ____  ____  __  ___________              __   __              
   / __ \/ __ \/  |/  / ____/ /_  ___  _____/ /__/ /_  ____  _  __
  / / / / / / / /|_/ / /   / __ \/ _ \/ ___/ //_/ __ \/ __ \| |/_/
 / /_/ / /_/ / /  / / /___/ / / /  __/ /__/ ,< / /_/ / /_/ />  <  
/_____/\____/_/  /_/\____/_/ /_/\___/\___/_/|_/_.___/\____/_/|_|  
 
*/
class DOMCheckbox :
	public virtual DOMNode,
	public virtual DOMCheckboxInteraction
{
public:
	DOMCheckbox(int id, TabDOMNodeInterface* pTab) :
		DOMNode(id), 
		DOMJavascriptCommunication(pTab), 
		DOMCheckboxInteraction() {}

	// Define initialization through ICP message in each DOMNode subclass
	virtual int Initialize(CefRefPtr<CefProcessMessage> msg) override;

	// Extract data from CefProcessMessage to C++ object's corresponding attibute
	virtual bool Update(DOMAttribute attr, CefRefPtr<CefListValue> data) override;

	// Build description
	static void GetDescription(std::vector< const std::vector<DOMAttribute>* >* descriptions) {
		super::GetDescription(descriptions);
		descriptions->push_back(&_description);
	}


	static const std::string GetJSObjectGetter()
	{
		return "GetDOMCheckbox";
	}

	virtual bool PrintAttribute(DOMAttribute attr);

	virtual int GetType() override { return 4; }

	bool GetCheckedState() const { return _checked; }

private:
	typedef DOMNode super;


	// Setter
	void SetCheckedState(bool state) { _checked = state; }

	bool IPCSetCheckedState(CefRefPtr<CefListValue> data);

	// Members
	bool _checked = false;


	static const std::vector<DOMAttribute> _description;
};

#pragma warning(pop) // warning about dominance inheritage

#endif  // DOMNODE_H_
