//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
//============================================================================

// Global variables in which DOM relevant information is stored
window.fixed_elements = new Set();
window.fixed_IDlist = [];				// Position |i| contains boolean information if ID |i| is currently used
window.fixed_coordinates = [[]];		// 2D list of bounding rectangle coordinates, fill with bounding rect coordinates of fixed node and its children (if position == 'relative')

window.dom_links = [];
window.dom_links_rect = [[]];

window.dom_textinputs = [];
window.dom_textinputs_rect = [[]];

window.appendedSubtreeRoots = new Set();


function DrawRect(rect, color)
{
	//Position parameters used for drawing the rectangle
	var x = rect[1];
	var y = rect[0];
	var width = rect[3] - rect[1];
	var height = rect[2] - rect[0];

	var canvas = document.createElement('canvas'); //Create a canvas element
	//Set canvas width/height
	canvas.style.width='100%';
	canvas.style.height='100%';
	//Set canvas drawing area width/height
	canvas.width = window.innerWidth;
	canvas.height = window.innerHeight;
	//Position canvas
	canvas.style.position='absolute';
	canvas.style.left=0;
	canvas.style.top=0;
	canvas.style.zIndex=100000;
	canvas.style.pointerEvents='none'; //Make sure you can click 'through' the canvas
	document.body.appendChild(canvas); //Append canvas to body element
	var context = canvas.getContext('2d');
	//Draw rectangle
	context.rect(x, y, width, height);
	context.fillStyle = color;
	context.fill();
}

function DrawObject(obj)
{
	var colors = ["#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#FFFFFF"];
	for(var i = 0; i < obj.rects.length; i++)
	{
		DrawRect(obj.rects[i], colors[i % 6]);
	}
}

// Helper function for console output
function ConsolePrint(msg)
{
	window.cefQuery({ request: (""+msg), persistent : false, onSuccess : function(response) {}, onFailure : function(error_code, error_message){} });
}

function SendLSLMessage(msg) {
    window.cefQuery({ request: ("lsl:" + msg), persistent: false, onSuccess: function (response) { }, onFailure: function (error_code, error_message) { } });
}

function LoggingMediator()
{
	/* This function is indirectly called via this.log */
    this.logFunction = null;

	/* Register your own log function with this function */
    this.registerFunction = function(f)
    {
        this.logFunction = f;
    }

    /* Unregister the log function with this function */
    this.unregisterFunction = function() {
        this.logFunction = null;
    }

	/* This function is called by CEF's renderer process */
    this.log = function(logText)
    {
        try
        {
            if(this.logFunction !== null)
                this.logFunction(logText);
        }
        catch(e)
        {
            console.log("LoggingMediator: Something went wrong while redirecting logging data.");
            console.log(e);
        }
    }

    /* Code, executed on object construction */
    ConsolePrint("LoggingMediator instance was successfully created!");

}

window.loggingMediator = new LoggingMediator();



function GetTextSelection()
{
	// Pipe message to C++ MsgRouter
	ConsolePrint("#select#"+document.getSelection().toString()+"#");
}
// TODO: Add as global function and also use it in DOM node work
/**
	Adjust bounding client rectangle coordinates to window, using scrolling offset and zoom factor.

	@param: DOMRect, as returned by Element.getBoundingClientRect()
	@return: Double array with length = 4, containing coordinates as follows: top, left, bottom, right
*/
function AdjustRectCoordinatesToWindow(rect)
{
	if(rect === undefined)
		ConsolePrint("WARNING: rect == undefined in AdjustRectCoordinatesToWindow!");

	var doc = document.documentElement;
	var zoomFactor = 1;
	var offsetX = (window.pageXOffset || doc.scrollLeft) - (doc.clientLeft || 0); 
	var offsetY = (window.pageYOffset || doc.scrollTop) - (doc.clientTop || 0); 

	// var docRect = document.body.getBoundingClientRect(); // not used

	if(document.body.style.zoom)
	{
		zoomFactor = document.body.style.zoom;
	}

	var output = [];
	output.push(rect.top*zoomFactor + offsetY);
	output.push(rect.left*zoomFactor + offsetX);
	output.push(rect.bottom*zoomFactor + offsetY);
	output.push(rect.right*zoomFactor + offsetX);
	return output;
}


// parent & child are 1D arrays of length 4
function ComputeBoundingRect(parent, child)
{
	// top, left, bottom, right

	var bbs = [];
	for(var i = 0, n=child.length/4; i < n; i++)
	{
		var _child = [child[i], child[i+1], child[i+2], child[i+3]];

		// // no intersection possible
		// if(parent[3] <= _child[0] || parent[3] <= _child[1])
		// 	bbs.push(_child);

		if(_child[2] - _child[0] > 0 && _child[3] - _child[1])
			bbs.push(_child);
	}

	return bbs;
}


function CompareArrays(array1, array2)
{
	var n = array1.length;

	if(n != array2.length)
		return false;

	for(var i = 0; i < n; i++)
	{
		if(array1[i] != array2[i])
			return false;
	}

	return true;
}

// TESTING PURPOSE
document.onclick = function(){
	// ConsolePrint("### document.onclick() triggered! Calling UpdateDOMRects! ###");
	// UpdateDOMRects();
	
}
// TESTING TWITTER, no updates through clicking right now
// document.addEventListener("click", function(e){
// 	ConsolePrint("JS Debug: Realized that click event was fired! Target is listed in DevTools Console.");
// 	console.log("Clicked target: "+e.target);
// 	UpdateDOMRects();
// });

// Trigger DOM data update on changing document loading status
document.onreadystatechange = function()
{
	// ConsolePrint("### DOCUMENT STATECHANGE! ###");

	// if(document.readyState == 'loading')
	// {
	// 	// ConsolePrint('### document.readyState == loading ###'); // Never triggered
	// }

	if(document.readyState == 'interactive')
	{
		// GMail fix
		ForEveryChild(document.documentElement, AnalyzeNode);

		UpdateDOMRects();

		// GMail
		// ForEveryChild(document.documentElement, AnalyzeNode);
	}

	if(document.readyState == 'complete')
	{
		// GMail fix
		ForEveryChild(document.documentElement, AnalyzeNode);

		UpdateDOMRects();

		ConsolePrint('<----- Page fully loaded. -------|');

		// GMail
		// ForEveryChild(document.documentElement, AnalyzeNode);
		
	}
}


window.onresize = function()
{
	//UpdateDOMRects();
	// TODO: Update fixed elements' Rects too?
	ConsolePrint("Javascript detected window resize, update of fixed element Rects.");

	// UpdateFixedElementRects(); // TODO: Already triggered by CEF's JS injection?
}


document.addEventListener('transitionend', function(event){
	// Tree, whose children have to be check for rect updates
	var tree = event.target.parentNode || event.target;

	// If first node is fixed element, only call fixed objects update rects method
	// it triggers rect updates for all its children
	if((fixedObj = GetFixedElementByNode(tree)) !== undefined && fixedObj !== null)
	{
		fixedObj.updateRects();
	}
	else
	{
		ForEveryChild(tree, function(child)
		{
			/* Update every child after transition took place */
			if(child.nodeType === 1)
			{
				// Check for DOMObjects
				var type = child.getAttribute("nodeType");
				if(type !== undefined && type !== null)
				{
					var id = child.getAttribute("nodeID");
					var domObj = GetDOMObject(type, id);
					if(domObj !== null && domObj !== undefined)
					{
						domObj.updateRects();
					}
				}

				var overflowId = child.getAttribute("overflowId");
				if(overflowId !== undefined && overflowId !== null)
				{
					var overflowObj = GetOverflowElement(overflowId);
					if(overflowObj !== undefined && overflowObj !== null)
					{
						overflowObj.updateRects();
					}
				}

				if(child.hasAttribute("fixedId") !== undefined)
				{
					var fixedObj = GetFixedElementByNode(child);
					if(fixedObj !== undefined && fixedObj !== null)
					{
						fixedObj.updateRects();
					}
				}




			}
		}); // ForEveryChild end
	}

}, false);



var docFrags = [];

var origCreateElement = Document.prototype.createElement;
Document.prototype.createElement = function(tag)
{
	if(arguments[0] === "iframe" || arguments[0] === "IFRAME") 
	{
		// ConsolePrint("<iframe> created.");
	}
	// ConsolePrint("Creating element with tagName="+tag);

	var elem = origCreateElement.apply(this, arguments);
	// if(arguments[0] === "iframe") elem.style.backgroundColor = "#0000ff";
	return elem ;
}

var origImportNode = Document.prototype.importNode;
Document.prototype.importNode = function(importedNode, deep){
	ConsolePrint("Document.importNode called!");
	return origImportNode.apply(this, arguments);
}

// DEBUG
// _send = XMLHttpRequest.prototype.send;
// XMLHttpRequest.prototype.send = function() {

//     /* Wrap onreadystaechange callback */
//     var callback = this.onreadystatechange;
//     this.onreadystatechange = function() {             
//          if (this.readyState == 4) {

//              /* We are in response; do something, like logging or anything you want */
// 				 console.log("XMLHttpRequest: Response type is "+this.responseType )

// 			//  console.log("response: "+this.responseText);
//          }

//          callback.apply(this, arguments);
//     }

//     _send.apply(this, arguments);
// }

/* Modify appendChild in order to get notifications when this function is called */
var originalAppendChild = Element.prototype.appendChild;
Element.prototype.appendChild = function(child){

	// appendChild extension: Check if root is already part of DOM tree
    if(this.nodeType == 1 || this.nodeType > 8)
    {
		// DEBUG0
		// if(child.tagName === "IFRAME")
		// {
		// 	console.log("Found iframe!");
		// 	console.log(child);
		// 	return;
		// }

		var subtreeRoot = this;

		// Stop going up the tree when parentNode is documentElement or doesn't exist (null or undefined)
		while(subtreeRoot !== document.documentElement && subtreeRoot.parentNode && subtreeRoot.parentNode !== undefined)
		{
			subtreeRoot = subtreeRoot.parentNode;
		}


		// Register subtree roots, whose children have to be checked outside of MutationObserver
        if(subtreeRoot !== document.documentElement) 
		{

			// When DocumentFragments get appended to DOM, they "lose" all their children and only their children are added to DOM
			if(subtreeRoot.nodeType == 11) // 11 == DocumentFragment
			{
				// Mark all 1st generation child nodes of DocumentFragments as subtree roots
				for(var i = 0, n = subtreeRoot.childNodes.length; i < n; i++)
				{
					window.appendedSubtreeRoots.add(subtreeRoot.childNodes[i]);

					ForEveryChild(subtreeRoot.childNodes[i], function(childNode){
						// if(childNode.style) childNode.style.backgroundColor = '#0000ff';
					});
					
				}
			}
			else 
			{	
				// if(subtreeRoot.style)
				// 	subtreeRoot.style.backgroundColor = "#ff0000";

				// Add subtree root to Set of subtree roots
				window.appendedSubtreeRoots.add(subtreeRoot);	

				// Remove children of this subtree root from subtree root set --> prevent double-checking of branches
				ForEveryChild(subtreeRoot, function(childNode){
						// if(childNode.nodeType == 1 && childNode.style) childNode.style.backgroundColor = '#ffff00';
						window.appendedSubtreeRoots.delete(childNode);
					}
				);


			}
		
		}

    }  

	// DocumentFragment as parent: children disappear when fragment is appended to DOM tree
	if(child.nodeType === 11)
	{
		// console.log("Document Fragment was appended to ", this);

		// ConsolePrint("DocumentFragment as child (appendChild)");
		for(var i = 0, n = child.childNodes.length; i < n; i++)
		{

			// COLORING
			// if(child.childNodes[i].style) child.childNodes[i].style.backgroundColor = "#ff0000";
			ForEveryChild(child.childNodes[i], function(childNode){
					// if(childNode.nodeType == 1 && childNode.style) childNode.style.backgroundColor = '#ff00ff';
					window.appendedSubtreeRoots.delete(childNode);
				}
			);

			window.appendedSubtreeRoots.add(child.childNodes[i]);
		}
	}

	// Finally: Execute appendChild
    return originalAppendChild.apply(this, arguments); // Doesn't work with child, why?? Where does arguments come from?
};

Document.prototype.createDocumentFragment = function() {
	var fragment = new DocumentFragment();

	// TODO: childList in config not needed? Or later for children, added to DOM, relevant?

	var config = { attributes: true, childList: true, characterData: true, subtree: true, characterDataOldValue: false, attributeOldValue: true};
	window.observer.observe(fragment, config);

	return fragment;
};


function ForEveryChild(parentNode, applyFunction, abortFunction)
{
	var childs = parentNode.childNodes;

	// Abort further processing of child nodes if abort conditions are met
	if(abortFunction !== undefined && abortFunction(parentNode))
	{
		return;
	}


	if(childs && applyFunction)
	{
		var n = childs.length;
		for(var i = 0; i < n; i++)
		{
			applyFunction(childs[i]);

			ForEveryChild(childs[i], applyFunction, abortFunction);
		}
	}

}

function AnalyzeNode(node)
{
	if( (node.nodeType == 1 || node.nodeType > 8) && (node.hasAttribute && !node.hasAttribute("nodeType")) && (node !== window)) // 1 == ELEMENT_NODE
	{
		if(window.appendedSubtreeRoots.delete(node))
		{
			ForEveryChild(node, AnalyzeNode);
		}

		var computedStyle = window.getComputedStyle(node, null);

		// Identify fixed elements on appending them to DOM tree
		if(
			// computedStyle.getPropertyValue('display') !==  "none" && // NOTE: if display == 'none' -> rect is zero
			(
				computedStyle.getPropertyValue('position') == 'fixed'
				//  ||	node.tagName == "DIV" && node.getAttribute("role") == "dialog"	// role == dialog isn't sufficient
			)
		) 
		{
			// Returns true if new FixedElement was added; false if already linked to FixedElement Object
			if(AddFixedElement(node))
			{
				UpdateDOMRects();
			}
		}


		if(node.tagName === "SELECT")
		{

			CreateDOMSelectField(node);
		}


		// Find text links
		if(node.tagName == 'A' )
		{
			CreateDOMLink(node);
		}

		if(node.tagName == 'INPUT' || node.tagName == "BUTTON") // Fun fact: There exists the combination of tag "BUTTON" and type "submit"
		{
			// Identify text input fields
			if(node.type == 'text' || node.type == 'search' || node.type == 'email' || node.type == 'password')
			{
				CreateDOMTextInput(node);
			}

			// TODO: Handle other kinds of <input> elements
			if(node.type == 'button' || node.type == 'submit' || node.type == 'reset' || !node.hasAttribute('type'))
			{
				// TODO: CreateDOMButton!
				CreateDOMLink(node);
			}
		}
		// textareas or DIVs, whole are treated as text fields
		if(node.tagName == 'TEXTAREA' || (node.tagName == 'DIV' && node.getAttribute('role') == 'textbox'))
		{
			CreateDOMTextInput(node);
		}

		// NEW: Buttons
		if(node.tagName == 'DIV' && node.getAttribute('role') == 'button')
		{
			CreateDOMLink(node);
		}

		// GMail
		if(node.tagName == 'DIV' && (node.getAttribute('role') == 'link' || node.getAttribute('role') == 'tab') )
		{
			CreateDOMLink(node);
		}
		if(node.tagName == "SPAN" && node.hasAttribute('email'))
		{
			CreateDOMLink(node);
		}

		// GMail: Trying to make mail receiver clickable (again)
		if(node.tagName === "DIV" && node.className === "vR")
		{
			console.log("GMail mail receiver fix: Found <div> with class 'vR'. I will assume it will be clickable.");
			CreateDOMLink(node);
		}
		if(node.tagName === "SPAN" && node.tagName === "aQ2")
		{
			console.log("GMail mail receiver fix: Found <span> with class 'aQ2'. I will assume it will be clickable.");
			CreateDOMLink(node);
		}
		if(node.tagName == "IMG" && node.getAttribute("data-tooltip") !== null)
		{
			CreateDOMLink(node);
		}

		var rect = node.getBoundingClientRect();

		// Detect scrollable elements inside of webpage
		if(node.tagName === "DIV" && rect.width > 0 && rect.height > 0)
		{
			var overflow = computedStyle.getPropertyValue("overflow");
			if(overflow === "auto" || overflow === "scroll")
			{
				CreateOverflowElement(node);
			}
			else
			{
				var overflowX = computedStyle.getPropertyValue("overflow-x");
				var overflowY = computedStyle.getPropertyValue("overflow-y");
				if(overflowX === "auto" || overflowX === "scroll" || overflowY === "auto" || overflowY === "scroll")
				{
					CreateOverflowElement(node);
				}
			}
		}


		// Update whole <form> if transition happens in form's subtree
		// (For shifting elements in form (e.g. Google Login) )
		if(node.tagName == 'FORM')
		{

			node.addEventListener('webkitTransitionEnd', function(event){
				// ConsolePrint("FORM transition event detected"); //DEBUG
				ForEveryChild(node, function(child){
					if(child.nodeType == 1)
					{
						var nodeType = child.getAttribute('nodeType');
						if(nodeType)
						{
							var nodeID = child.getAttribute('nodeID');
							var domObj = GetDOMObject(nodeType, nodeID);
							if(domObj)
							{
								domObj.updateRects();
							}
						}
					}
					
				});
			}, false);
		}


	}
}

window.debug = false;

// Instantiate and initialize the MutationObserver
function MutationObserverInit()
{ 
	ConsolePrint('Initializing Mutation Observer ... ');

	window.observer = new MutationObserver(
		function(mutations) {
		  	mutations.forEach(
		  		function(mutation)
		  		{
					if(debug)
					console.log(mutation.type, "\t", mutation.attributeName, "\t", mutation.oldValue, "\t", mutation.target);
					
					var working_time_start = Date.now();

			  		var node;
					
		  			if(mutation.type === 'attributes')
		  			{
		  				node = mutation.target;

						//   if(node.tagName === "SCRIPT")
						//   	return;

			  			var attr; // attribute name of attribute which has changed


		  				if(node.nodeType == 1) // 1 == ELEMENT_NODE
		  				{
		  					attr = mutation.attributeName;

							// Automatically fix or unfix children, if given attribute changed
							if(attr === "fixedid" || attr === "childfixedid")
							{
								
								var id = node.getAttribute(attr);
								var id_set = (id !== null && id !== undefined);

								// console.log(attr, "\t", id, "\t", id_set);
								
								var i = 0, n = node.childNodes.length;
								for(; i < n; i++)
								{
									var child = node.childNodes[i];
									if(child.nodeType === 1)
									{
										if(id_set)
										{
											child.setAttribute("childFixedId", id);
										}
										else
										{
											child.removeAttribute("childFixedId");
										}
										
										SetFixationStatus(child, id_set);
									}
								}
							}
							

		  					if(attr == 'style' ||  (document.readyState != 'loading' && attr == 'class') )
		  					{
								if(window.getComputedStyle(node, null).getPropertyValue('position') === 'fixed')
								{
									// ConsolePrint("Trying to AddFixedElement...");
									if(AddFixedElement(node))
									{
										// Update every Rect, just in case anything changed due to an additional fixed element
										UpdateDOMRects();
									}

								}
								else 
								{
									// Checks if node corresponds to fixedObj and removes it, when true
									RemoveFixedElement(node);
								}
							}


							if(attr == "style")
							{

								// Goal: Recognise changes in e.g. style.display
								// 'solution': Trigger rect update if changes in style took place. Direct change in style would be
								// value assignment, which will be recognised in MutationObserver
								UpdateNodesRect(node);
								UpdateChildrensDOMRects(node);

								// TODO: Changes in style may occure when scrolling some elements ... might be a lot of Rect Update calls!
								
		  					} // END attr == 'style'


							// Changes in attribute 'class' may indicate that bounding rect update is needed, if node is child node of a fixed element
							if(attr == 'class')
							{
								var parent_updated = false;
								var childFixedId = node.getAttribute("childFixedId");
								if(childFixedId !== null && childFixedId !== null)
								{
									var fixedObj = GetFixedElementById(childFixedId);
									if(fixedObj !== null && fixedObj !== undefined)
									{
										fixedObj.updateRects();
										UpdateChildrensDOMRects(fixedObj.node);
										parent_updated = true;
									}
								}

								if(!parent_updated)
								{
									var update_needed = false;

									if((fixedObj = GetFixedElementByNode(node)) !== null && fixedObj !== undefined)
									{
										update_needed |= fixedObj.updateRects()
									}

						
									// Update DOMObj / OverflowElement Rect, if node is linked to one
									// TODO: Simple UpdateRects for one DOMObj & OverflowElement?
									var nodeType = node.getAttribute('nodeType');
									if(nodeType !== null)
									{
										var nodeID = node.getAttribute('nodeID');
										var domObj = GetDOMObject(nodeType, nodeID);
										if(domObj !== null && domObj !== undefined)
										{
											// domObj.checkVisibility(); // included in updateRects()
											update_needed |= domObj.updateRects();

										}
									}

									var overflowId = node.getAttribute("overflowId");
									if(overflowId !== null)
									{
										var overflowObj = GetOverflowElement(overflowId);
										if(overflowObj !== null && overflowObj !== undefined)
										{
											update_needed |= overflowObj.updateRects();
										}
									}

									if(update_needed)
									{
										UpdateChildrensDOMRects(node);
									}

								} // !parent_updated
							}

		  				} // END node.nodeType == 1
						
					

		  			} // END mutation.type == 'attributes'





		  			if(mutation.type === 'childList') // TODO: Called upon each appending of a child node??
		  			{
						// console.log("type: ", mutation.type,"\ttarget: ", mutation.target, "\tnodeType: ", mutation.target.nodeType);

		  				// Check if fixed nodes have been added as child nodes
			  			var nodes = mutation.addedNodes;
						var parent = mutation.target;
						
			  			for(var i=0, n=nodes.length; i < n; i++)
			  			{

			  				node = nodes[i]; // TODO: lots of data copying here??
							// console.log("-- ", node);


							// if(node.nodeType === 11)
			  					// console.log("Document Fragment got appended to: ", parent);

							// if(mutation.target === "#document-fragment")
							// 	console.log("### WOW! ###");	  
						
							// Mark child nodes of DocumentFragment, in order to analyze their subtrees
							if(mutation.target.nodeType === 11)
							{
								// console.log("Node was appended to a Document Fragment! ", node);

								window.appendedSubtreeRoots.add(node);
								ForEveryChild(node, function(child){ window.appendedSubtreeRoots.delete(child); });
							}
							
			  				AnalyzeNode(node);

							if(parent.nodeType === 1 && node.nodeType === 1)
							{
								var id = parent.getAttribute("childfixedid");
								if(id === null || id === undefined)
								{
									id = parent.getAttribute("fixedid");
								}
								if(id !== null && id !== undefined)
								{
									node.setAttribute("childfixedid", id);
									SetFixationStatus(node, true);
								}
							}
	
					
			  			}

						// if(nodes.length > 0)
						// {
						// 	ConsolePrint("Child added, updating all childrens' rects.");
						// 	UpdateChildrensDOMRects(mutation.target);
							
						// }
						if(parent.nodeType === 1)
						{
							var fix_id = parent.getAttribute("childFixedId");
							if(fix_id !== null && fix_id !== undefined)
							{
								var fixObj = GetFixedElementById(fix_id);
								if(fixObj !== null && fixObj !== undefined)
								{
									fixObj.updateRects();
								}
							}
						}

						

			  			// Check if fixed nodes have been removed as child nodes and if yes, delete them from list of fixed elements
			  			var removed_nodes = mutation.removedNodes;
			  			for(var i=0, n=removed_nodes.length; i < n; i++)
			  			{
			  				node = removed_nodes[i];

			  				if(node.nodeType === 1)
			  				{
								RemoveFixedElement(node, false);

								var overflowId = node.getAttribute("overflowId");
								if(overflowId !== null)
								{
									RemoveOverflowElement(overflowId);
								}

								// TODO: How to trigger update/remove used DOM Objects?
			  				}

							UpdateNodesRect(node);
							// Trigger Rect Updates after removal of (several) node(s)
							UpdateChildrensDOMRects(node);
						
			  			}


		  			} // END mutation.type == 'childList'

					mutation_observer_working_time += (Date.now() - working_time_start);

						
		  		} // END forEach mutation


		 	);    
		}
	);



	// TODO:

	// characterData vs. attributes - one of those not neccessary?

	// attributeFilter -
	// Mit dieser Eigenschaft kann ein Array mit lokalen Attributnamen angegeben werden (ohne Namespace), wenn nicht alle Attribute beobachtet werden sollen.
	// --> nur relevante Attribute beobachten!

	ConsolePrint('MutationObserver successfully created! Telling him what to observe... ');
	ConsolePrint('Trying to start observing... ');
		
	// Konfiguration des Observers: alles melden - Änderungen an Daten, Kindelementen und Attributen
	var config = { attributes: true, childList: true, characterData: true, subtree: true, characterDataOldValue: false, attributeOldValue: true};

	// eigentliche Observierung starten und Zielnode und Konfiguration übergeben
	window.observer.observe(window.document, config);

	ConsolePrint('MutationObserver was told what to observe.');

	// TODO: Tweak MutationObserver by defining a more specific configuration



	
} // END OF MutationObserverInit()

MutationObserverInit();



function MutationObserverShutdown()
{
	window.observer.disconnect(); 

	delete window.observer;

	ConsolePrint('Disconnected and deleted MutationObserver! ');
}




var mutation_observer_working_time = 0;
var load_starting_time;

function StartPageLoadingTimer()
{
	load_starting_time = Date.now();
}

function StopPageLoadingTimer()
{
	var page_load_duration = Date.now() - load_starting_time;
	ConsolePrint("Page load took "+page_load_duration+"ms");
	ConsolePrint("MutationObserver operations took "+mutation_observer_working_time+"ms, "+
		(100*mutation_observer_working_time/page_load_duration)+"% of page load.");
}


// window.onchange = function(e){ConsolePrint("Window changes: "+e);};

// ConsolePrint("Creating window_observer...");
// window.window_observer = new MutationObserver(
// 		function(mutations) {
// 		  	mutations.forEach(
// 		  		function(mutation)
// 		  		{
// 					ConsolePrint("Mutation in window object detected!");

// 		  			if(mutation.attributeName == "document")
// 					{
// 						ConsolePrint("window object's 'document' attribute changed!");

// 						var config = { attributes: true, childList: true, characterData: true, subtree: true, characterDataOldValue: false, attributeOldValue: true};
// 						window.observer.observe(window.document, config);
// 					}
// 				}
// 			);
// 		}
// );
// var config = {attributes: true}
// window.window_observer.observe(window, config );
// ConsolePrint("... done");

