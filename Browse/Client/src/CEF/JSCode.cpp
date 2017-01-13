//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "JSCode.h"
#include <map>
#include <fstream>
#include <iostream> // as called from differen processes, one cannot simply use LogInfo / LogError :(

// Folder with external JavaScript code
const std::string src = CONTENT_PATH "/javascript/";

// List of all available JS code file paths and how to access them with JSCode enum
const std::map<JSFile, std::string> findJSFile =
{
    std::make_pair<JSFile, std::string>(REMOVE_CSS_SCROLLBAR, src + "remove_css_scrollbar.js"),
    std::make_pair<JSFile, std::string>(DOM_UPDATE_SIZES, src + "dom_update_sizes.js"),
    std::make_pair<JSFile, std::string>(DOM_FILL_ARRAYS, src + "dom_fill_arrays.js"),
    std::make_pair<JSFile, std::string>(FAVICON_GET_URL_AND_RESOLUTION, src + "favicon_get_url_and_resolution.js"),
    std::make_pair<JSFile, std::string>(FAVICON_CREATE_IMG, src + "favicon_create_img.js"),
    std::make_pair<JSFile, std::string>(FAVICON_COPY_IMG_BYTES_TO_V8ARRAY, src + "favicon_copy_img_bytes_to_v8array.js"),
    std::make_pair<JSFile, std::string>(MUTATION_OBSERVER_TEST, src + "mutation_observer_test.js"),
	std::make_pair<JSFile, std::string>(DOM_MUTATIONOBSERVER, src + "dom_mutationobserver.js"),
	std::make_pair<JSFile, std::string>(DOM_FIXED_ELEMENTS, src + "dom_fixed_elements.js")
};

std::string GetJSCode(JSFile file)
{
    if (findJSFile.find(file) != findJSFile.end())
    {
        const std::string filePath = findJSFile.at(file);
        std::ifstream t(filePath);
        if (t.is_open())
        {
            t.seekg(0, std::ios::end);
            size_t size = t.tellg();
            std::string buffer(size, ' ');
            t.seekg(0);
            t.read(&buffer[0], size);
            return buffer;
        }
        else
        {
            std::cout << "JSCode: Cannot open JS code file, path: " << filePath << std::endl;
            return "alert('JSCode: Cannot open JS code file');";
        }
    }
    else
    {
        std::cout << "JSCode: Cannot find the requested JS code file" << std::endl;
        return "alert('JSCode: Cannot find the requested JS code file');";
    }
}

std::string jsInputTextData(int inputID, std::string text, bool submit)
{
	std::string code = "var domObj = GetDOMObject(0," + std::to_string(inputID) + ");\
		domObj.setTextInput('" + text + "'," + std::to_string(submit) + ");";
	return code;
}

std::string jsFavIconUpdate(std::string oldUrl)
{
	std::string code = "for (i = 0; i < links.length; i++)\
		{\
			if(links[i].rel == 'icon' || links[i].rel == 'shortcut icon')\
			{\
				window.favIconUrl = links[i].href;\
			}\
		}"
			/*if(window.favIconUrl == '')\
			alert('(Update) Searched '+links.length+', have not found iconURL...');\*/
			"if(window.favIconUrl != " + oldUrl + ")\
			favIconImg.src = window.favIconUrl;\
		";
	return code;
}
