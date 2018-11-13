//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Abstract interfaces for interaction with DOM nodes.

#ifndef DOMNODEINTERACTION_H_
#define DOMNODEINTERACTION_H_

#include "src/State/Web/Tab/Interface/TabDOMNodeInterface.h"
#include "include/cef_browser.h"
#include "src/CEF/Data/DOMAttribute.h"
#include "src/Utils/Logger.h"
#include <functional>
#include <memory>

// TODO: Strange place (this header file) for BaseInterface?
class DOMBaseInterface : std::enable_shared_from_this<DOMBaseInterface>
{
public:

	// Required in order to being able to access corresponding JS DOM object
	virtual int GetId() = 0;
	virtual int GetType() = 0;

	std::shared_ptr<DOMBaseInterface> getBasePtr() { return shared_from_this(); }

	// Enable accessing attribute data while running the program
	virtual bool PrintAttribute(DOMAttribute attr) = 0;
};

/*
 * Guarantee, that DOMNode provides any informationen needed in order to contact
 * its corresponding JavaScript DOM node object
*/
class DOMJavascriptCommunication  : public virtual DOMBaseInterface
{
public:

	// Constructor
	DOMJavascriptCommunication(TabDOMNodeInterface* tab) :
        _pTab(tab) {}

	TabDOMNodeInterface* _pTab;


private:

    // Default constructor only for friends. Problem: Virtual classes have to call some
    // constructor, although never instantiated on their own. Finally utilized classes like
    // DOMTextInput are calling the "good" constructor with parameter. So below is never really used.
    DOMJavascriptCommunication();

    // This are the friends who have to access the default constructor
    friend class DOMTextInputInteraction;
    friend class DOMOverflowElementInteraction;
    friend class DOMSelectFieldInteraction;
	friend class DOMVideoInteraction;
	friend class DOMCheckboxInteraction;

};

// Interaction with text input
class DOMTextInputInteraction : public virtual DOMJavascriptCommunication
{
public:

	// Constructor
    DOMTextInputInteraction() {}

	// Send IPC message to JS in order to execute text input function
	void InputText(std::string text, bool submit);
};

// Interaction with overflow element
class DOMOverflowElementInteraction : public virtual DOMJavascriptCommunication
{
public:

	// Constructor
    DOMOverflowElementInteraction() {}

	// TODO taking gaze, should take scrolling offset
	// Send IPC message to JS in order to execute scrolling function
	void Scroll(int x, int y, std::vector<int> fixedIds = {}) 
	{
		_pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "scroll", x, y, fixedIds); 
	}
};

// Interaction with select field
class DOMSelectFieldInteraction : public virtual DOMJavascriptCommunication
{
public:

	// Constructor
    DOMSelectFieldInteraction() {}

	// Send IPC message to JS in order to execute JS function
	void SetSelectionIndex(int idx) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "setSelectionIdx", idx); }
};

// Interaction with videos
class DOMVideoInteraction : public virtual DOMJavascriptCommunication
{
public:

	// Constructor
	DOMVideoInteraction() {}

	// Send IPC message to JS in order to execute JS function
	void JumpToSecond(float sec = 0.f) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "jumpToSecond", sec); }
	void SkipSeconds(float sec = 0.f) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "skipSeconds", sec); }
	void SetPlaying(bool playing = true) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "setPlaying", playing); }
	void SetMuted(bool muted = true) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "setMuted", muted); }
	void SetVolume(float volume) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "setVolume", volume); }
	void ShowControls(bool show = true) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "showControls", show); }
	void SetFullscreen(bool fullscreen = true) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "setFullscreen", fullscreen); }
	void ToggleMuted() { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "toggleMuted"); }
	void TogglePlayPause() { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "togglePlayPause"); }
	void ChangeVolume(float delta) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "changeVolume", delta); }
};

class DOMCheckboxInteraction : public virtual DOMJavascriptCommunication
{
public:
	DOMCheckboxInteraction() {}

	void SetChecked(bool state) { _pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "setChecked", state); }
};


#endif // DOMNODEINTERACTION_H_
