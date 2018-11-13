//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Handles JavaScript code.

#ifndef CEF_JSCODE_H_
#define CEF_JSCODE_H_

#include <string>

/**
*	This class loads externally saved Javascript code from .js files in CONTENT_PATH/javascript
*	and returns them as std::string
*	USAGE GUIDE:
*	1.)	Save your Javascript code to the mentioned file location
*	2.) Add a corresponding enum name to JSFile enum
*	3.)	Add a pair (JSFile, file name) to std::map findJSFile
*	4.) Include JSCode.h to your C++ class
*	5.) Add a constant std::string member to your class and initialize it with GetJSCode function
*	6.)	Inject your Javascript code by using your new member variable
*
*/

enum JSFile
{
    REMOVE_CSS_SCROLLBAR,
	// MutationObserver & DOMNodes
	DOM_MUTATIONOBSERVER,
	DOM_NODES,
	DOM_NODES_INTERACTION,
	DOM_FIXED_ELEMENTS,
	HELPERS,
	DOM_ATTRIBUTES,
	DOM_NODES_HELPERS
};

std::string GetJSCode(JSFile file);
std::string jsInputTextData(int inputID, std::string text, bool submit = false);
std::string jsFavIconUpdate(std::string oldUrl);

#endif // CEF_JSCODE_H_
