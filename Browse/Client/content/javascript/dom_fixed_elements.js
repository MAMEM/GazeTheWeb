//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
//============================================================================

ConsolePrint("Starting to import dom_fixed_elements.js ...");

window.domFixedElements = [];
/**
 * REFACTORING TODOs
 *  - Automatically add FixedElement to list of fixed elements after it's creation
 *  - On creation: Check if node already belongs to an fixed element object? OR Use CreateFixedElement function
 */


function FixedElement(node)
{
    /* Code executed on constructor call ... */
    // Add FixedElement Object to list and determine its id
    if(GetFixedElementByNode(node) !== undefined || typeof(node.setAttribute) !== "function")
        return false;

    window.domFixedElements.push(this);
    this.id = window.domFixedElements.indexOf(this);

    /* Attributes */
    this.node = node;
    this.rects = [];
    
    this.node.setAttribute("fixedId", this.id);

    // Set childFixedId for 1st generation of child nodes, rest will be done by MutationObserver
    var scoped_id = this.id;
    ForEveryChild(this.node, (child) => {
        if(typeof(child.setAttribute) !== "function")
            return;
        
        child.setAttribute("childFixedId", scoped_id);
    });
    
    // Compute fixed subtree's rects and inform CEF
    this.updateRects();
}

FixedElement.prototype.getId = function(){
    return this.id;
}

FixedElement.prototype.getRects = function(){
    return this.rects;
}

FixedElement.prototype.updateRects = function(){
    var cs = window.getComputedStyle(this.node, null);
    // Delete fixed element object, if position isn't fixed anymore
    if(cs.getPropertyValue("position") !== "fixed" || cs.getPropertyValue("display") == "none")
    {
        RemoveFixedElement(this.node);
        return true;
    }

    var updatedRectsData = [];

    // NOTE: Fix for GMail. There exist DIVs without any children, whose rects cover the whole page and they are fixed
    // although this does not seem to influence anything
    if((this.node.tagName === "DIV" && this.node.children.length === 0)
        || ( window.getComputedStyle(this.node, null).getPropertyValue("display") === "none")) // 9gag inactivity overlay
    {
        var previous_rects = this.rects;
        this.rects = [[0,0,0,0]];
        return !EqualClientRectsData(this.rects, previous_rects);
    }

    // Compute fitting bounding box for fixed node and all its children
    // Starting with ClientRects of this parent node
    var domRectList = [];
    for(var i = 0, rects = this.node.getClientRects(); i < rects.length; i ++)
    {
        domRectList.push(rects[i]);
    }
    
    
    // Check if child is already contained in one of the DOMRects
    ForEveryChild(
        this.node, 
        // Main function
        (child) => {
            var domObj = GetCorrespondingDOMObject(child);
            if(domObj !== undefined)
                domObj.updateRects();
            

            if(child.nodeType === 1 && 
                window.getComputedStyle(child, null).getPropertyValue("opacity") !== "0" &&
                window.getComputedStyle(child, null).getPropertyValue("visibility") !== "hidden")
            {
                var cr = child.getClientRects();
                for(var i = 0, n = cr.length; i < n; i++)
                {
                    var rect = cr[i];
                    var contained = false;

                    // Iterate over all current bounding rects
                    domRectList.forEach((container) => {
                            if(!contained) // Stop checking if rect is contained, when already contained in another rect
                                contained = IsRectContained(rect, container);
                        }
                    );

                    // Another rect must be added if current rect isn't contained in any 
                    if(!contained)
                        domRectList.push(rect);

                } // rects.forEach
            } // if nodeType === 1
        },
        // Abort function, executed after main function
        (node) => { 
            if (node.nodeType === 1)
            {
                var cs = window.getComputedStyle(node, null);
                // Skip children if invisible
                if(cs.getPropertyValue("opacity") === 0)
                    return true;
                // ... or overflow element, which would cover children anyway outside of rect
                var hiding = ["hidden", "scroll", "auto"]
                if(hiding.indexOf(cs.getPropertyValue("overflow")) !== -1)
                {
                    // ... but keep updating child rects, if registered DOMObject!
                    ForEveryChild(node, (child) => {
                        var domObj = GetCorrespondingDOMObject(child);
                        if(domObj !== undefined)
                            domObj.updateRects();
                    });
                    // ... then, abort.
                    return true;
                }
                return false;
            } 
            else 
                return true;
        }
    ); // ForEveryChild

    // Convert DOMRects to [t,l,b,r] float lists and adjust coordinates if zoomed
    domRectList.forEach((domRect) => { 
            updatedRectsData.push(AdjustRectToZoom(domRect));
        }
    );
            
    // Check if Rect data changed, if yes, inform CEF about changes
    var changed = !EqualClientRectsData(this.rects, updatedRectsData);

    // Save updated rect data in FixedElement objects attribute
    this.rects = updatedRectsData;

    if(changed)
        // Inform CEF that fixed element has been updated
        ConsolePrint("#fixElem#add#"+this.id+"#");

    return changed;
}

// TODO: Get rid of this function and only use object constructor?
function AddFixedElement(node)
{
    if(typeof(node.hasAttribute) !== "function")
        return false;

    if(node.hasAttribute("childFixedId"))
        return false;

    if(node.hasAttribute("fixedId"))
    {
        var id = node.getAttribute("fixedId")
        var fixedObj = GetFixedElementById(id);
        // Trigger rect updates of whole subtree, just in case
        if(fixedObj !== undefined)
            fixedObj.updateRects();

        return false;
    }
    else
    {
        // Create new FixedElement object, which will be added to global fixed element list
        new FixedElement(node);

        return true;
    }
    
}

function RemoveFixedElement(node)
{
    var fixedObj = GetFixedElementByNode(node);
    if(fixedObj === undefined)
        return false;
    
    // Needed for output at the end
    var id = fixedObj.getId();

    // Delete object in its list slot, slot will be left empty (undefined) at the moment
    if(id >= 0 && id < window.domFixedElements.length)
    {
        delete window.domFixedElements[id];
        ConsolePrint("#fixElem#rem#"+id);
    }

    node.removeAttribute("fixedId");

    // Set childrens fixedIds to this node's parent fixed id, if any
    var childFixedId = node.getAttribute("childFixedId");
    if(childFixedId)
    {
        node.childNodes.forEach((child) => {
            if(typeof(child.setAttribute) === "function")
                child.setAttribute("childFixedId", childFixedId);
        });
    }
    else
    {
        node.childNodes.forEach((child) => {
            if(typeof(child.removeAttribute) === "function")
                child.removeAttribute("childFixedId");
        });
    }
    
    // Just in case
    // UpdateDOMRects("RemoveFixedElement");
    UpdateChildNodesRects(node.parent);
}


function AdjustRectToZoom(rect)
{

	var zoomFactor = 1.0;

	if(document.body.style.zoom)
	{
		zoomFactor = document.body.style.zoom;
	}

	var output = [];
	output.push(rect.top*zoomFactor);
	output.push(rect.left*zoomFactor);
	output.push(rect.bottom*zoomFactor);
	output.push(rect.right*zoomFactor);

	return output;
}

// TODO: move to geometry_helpers.js or something like that
function IsRectContained(rect, container)
{
    if(rect.width === 0 || rect.height === 0)
        return true;
    
    if(container.left <= rect.left && container.right >= rect.right
        && container.top <= rect.top && container.bottom >= rect.bottom)
        return true;

    // TODO: Idea: If rect partially contained, "cut off" not contained part and check if other containers contain it
    // if not: add "cut off" part to containers

    return false;
}
ConsolePrint("Successfully imported dom_fixed_elements.js!");