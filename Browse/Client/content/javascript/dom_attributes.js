//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
//============================================================================

ConsolePrint("Starting to import dom_attributes.js ...");

// Filled using C++
window.attrStrToInt = new Map();
function AddDOMAttribute(attrStr, attrInt)
{
    if(typeof(attrStr) !== "string" || typeof(attrInt) !== "number")
    {
        console.log("Error: Invalid datatypes given when callin AddDOMAttribute.\nattrStr: "+attrStr+"\nattrInt: "+attrInt);
        return;
    }

    window.attrStrToInt.set(attrStr, attrInt);
}

function GetAttributeCode(attrStr)
{
    var attrInt = window.attrStrToInt.get(attrStr);
    if(attrInt === undefined)
        console.log("Error in GetAttributeCode: Tried to handle an attribute called '"+attrStr+"', but is unknown.");
    return attrInt;
}


/**
 * Fetch attribute, named |attrStr|, data by calling |domObj|s getter domObj['get'+|attr|]();
 */
function SendAttributeChangesToCEF(attrStr, domObj)
{
    if (typeof(domObj.getType) !== "function")
        return "Invalid 'getType' function!";

    if(typeof(domObj.isCppReady) !== "function" || !domObj.isCppReady())
        return "Node not yet ready on C++ side!";

    if(domObj.getCefHidden())
        return "Given domObj with id: "+domObj.getId()+" & type: "+domObj.getType()+" is CefHidden!";
    
    var attrCode = GetAttributeCode(attrStr);
    if(attrCode === undefined)
        return "Invalid attrCode: "+attrCode;

    var encodedData = FetchAndEncodeAttribute(domObj, attrStr);
    if (encodedData === undefined)
    {
        console.log("Error in SendAttributeChangesToCEF: Failed to encode attribute "+attrStr+"!");
        return "Invalid encoded data: "+encodedData;
    }
    
    var msg = "DOM#upd#"+domObj.getType()+"#"+domObj.getId()+"#"+attrCode+"#"+encodedData+"#";

    ConsolePrint(msg);
    // console.log("AttrChanges: "+msg);
    return "Success, sent: "+msg;
}


// TODO: Automatically decide which kind of encoding will take place? (typeof(true) === "boolean")
window.attrStrToEncodingFunc = new Map();
var simpleReturn = (data) => { return data; };
var listReturn = (data) => { return data.reduce( (i,j) => {return i+";"+j; })};
var bitmaskReturn = (data) => {return data.reduce( (i,j) => {return i+j;}, "" )};

window.attrStrToEncodingFunc.set("Rects", (data) => {
    if(data.length === 0)
        return [0,0,0,0];
    // Add ';' between every element in each list, afterwards add ';' between each list
    // FUTURE TODO: Use another kind of delimiter in between list elements as between lists
    return data.map( (l) => { return l.reduce((n,m) => {return n+";"+m; }) }).reduce( (i,j) => {return i+";"+j; });
    }
);
window.attrStrToEncodingFunc.set("FixedId", simpleReturn);
window.attrStrToEncodingFunc.set("OverflowId", simpleReturn);
window.attrStrToEncodingFunc.set("Text", simpleReturn);
window.attrStrToEncodingFunc.set("IsPassword", (data) => {return data + 0; });  // implicity cast bool to number
window.attrStrToEncodingFunc.set("Url", simpleReturn);
window.attrStrToEncodingFunc.set("Options", listReturn);
window.attrStrToEncodingFunc.set("MaxScrolling", listReturn);
window.attrStrToEncodingFunc.set("CurrentScrolling", listReturn);
window.attrStrToEncodingFunc.set("OccBitmask", bitmaskReturn);
window.attrStrToEncodingFunc.set("HTMLId", simpleReturn);
window.attrStrToEncodingFunc.set("HTMLClass", simpleReturn);
window.attrStrToEncodingFunc.set("Checked", simpleReturn);

function FetchAndEncodeAttribute(domObj, attrStr)
{
    var encodeFunc = window.attrStrToEncodingFunc.get(attrStr);
    if(encodeFunc === undefined)
    {
        console.log("Error in FetchAndEncodeAttribute: There does not exist any encoding function for attribute '"+attrStr+"'!");
        return undefined;
    }

    // Definition: Each getter should be called getAttrStr for simplicity
    if(typeof(domObj["get"+attrStr]) !== "function")
    {
        console.log("Error in FetchAndEncodeAttribute: Could not find function 'get"+attrStr+"' in given DOM object!", domObj);
        return undefined;     
    }

    // Call attribute getter
    var data = domObj["get"+attrStr](false); // param update=false

    // Enode and return data
    return encodeFunc(data);
}



ConsolePrint("Successfully imported dom_attributes.js!");