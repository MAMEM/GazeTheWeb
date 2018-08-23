//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
//============================================================================
ConsolePrint("Starting to import dom_mutationobserver.js ...");

// Log which type of HTML element was clicked
document.onclick = function(e){
	console.log("Javascript realized click! Event available as var last_click_event");
	window.last_click_event = e;

	if (e && e.target && e.target.tagName)
	{
	    SendDataMessage(e.target.tagName.toLowerCase() + "," + e.target.id + "," + e.pageX + "," + e.pageY);
	}
}

document.addEventListener("auth.statusChange", (e) => {
	console.log("Noticed auth.statusChange event!");	
	ConsolePrint("Noticed auth.statusChange event!");	
}, false);
// window.appendedSubtreeRoots = new Set();

// Trigger DOM data update on changing document loading status
document.onreadystatechange = function()
{
	console.log("document.readyState == "+document.readyState);
	ConsolePrint("document.readyState == "+document.readyState);

	// TODO: First change of OccBitmask isn't recognized by CEF!
	if(document.readyState === "loading" || document.readyState === "complete")
	{
		window.domNodes.forEach((list) => {
			// Force send message about attribute changes
			list.forEach((o) => { SendAttributeChangesToCEF("OccBitmask", o); });
		});
	}

	if(document.readyState === "complete")
	{
		ForEveryChild(document.documentElement, AnalyzeNode);

		var keywords = document.querySelector("meta[name='keywords']");
		if(keywords && keywords.content)
		{
			SendToMsgRouter("#meta#keywords#"+keywords.content+"#");
		}
		else
		{
			SendToMsgRouter("#meta#keywords##");
		}
	}

	// BUGFIX
	// if (document.readyState === "complete")
	// 	ForEveryChild(document.documentElement, AnalyzeNode);

	// domOverflowElements.forEach(
	// 	(o) => { o.checkIfOverflown(); }
	// );

	// if(document.readyState === "complete")
	// {
	// 	console.log("Marking all available nodes...");
	// 	ForEveryChild(document.documentElement, (n) => {n.seen = true; })
	// }

	// INFO: Not analyzed nodes are more commenly added after page load completion!
	//if(document.readyState === "complete")
		// CountAnalyzedNodes();
}

window.onwebkitfullscreenchange = function()
{
	UpdateDOMRects("onwebkitfullscreenchange");
}

window.onresize = function()
{
	//UpdateDOMRects();
	// TODO: Update fixed elements' Rects too?
	ConsolePrint("Javascript detected window resize, update of fixed element Rects.");
}


// Update rects if CSS transition took place (TODO: Needed if parent's rects didn't change?)
document.addEventListener('transitionend', function(event){
	// Tree, whose children have to be check for rect updates
	var root = event.target;

	// TODO: Hiding reason should be shared with children, altough
	var fixedElem = GetFixedElementByNode(root);
	if(fixedElem !== undefined)
	{
		// If root is fixed element, subtree will be updated by simply calling updateRects
		fixedElem.updateRects();
		return;
	}

	// DOMObjects might not be visible if any parent has opacity == 0, so store information about
	// possible hidding parents as node attributes, check these in DOMNode.updateRects
	var hiding_reason = undefined;
	var cs = window.getComputedStyle(root, null);
	if(cs.getPropertyValue("opacity") === "0")
		hiding_reason = "opacity";

	// Update rects of given root node
	var obj = GetCorrespondingDOMObject(root);
	if(obj !== undefined)
		obj.updateRects();

	// Update rects of whole subtree beneath root node
	ForEveryChild(root, 
		(child) => {
			if(typeof(child.getAttribute) !== "function")
				return;

			if(hiding_reason !== undefined && child.hidden_by === undefined)
				child.hidden_by = new Map();

			if(hiding_reason === undefined && child.hidden_by !== undefined && child.hidden_by.has(root))
				child.hidden_by.delete(root);

			if(hiding_reason !== undefined)
				child.hidden_by.set(root, hiding_reason);


			var fixedElem = GetFixedElementByNode(child);
			if(fixedElem === undefined)
			{
				var obj = GetCorrespondingDOMObject(child);
				if (obj !== undefined)
				{
					obj.updateRects();
				}
			}
			else
				fixedElem.updateRects();
		},
		(child) => {
			// Abort rect update of childs subtree, if child is fixed element
			// With child as fixed element, rect updates for subtree will be triggered anyway
			return (GetFixedElementByNode(child) !== undefined);
		}
	); // ForEveryChild end

}, false);



var docFrags = [];
var debug_nodes = [];

var appended_nodes = new Set();
/* Modify appendChild in order to get notifications when this function is called */
var originalAppendChild = Element.prototype.appendChild;
Element.prototype.appendChild = function(child){
	// appendChild extension: Check if root is already part of DOM tree
	var previous_root = this.getRootNode();

	// BUGFIX
	// Check which nodes where appended and which will trigger childList mutation, some won't.
	// Current approach: Analyze those in CefPoll function
	appended_nodes.add(appended_nodes); 
	
	// DEBUG
	// if(this.nodeType === 11)
	// 	console.log("Appending child to document fragment (idx: "+document_fragments.indexOf(this)+")")
	// if(child.nodeType === 11)
	// 	console.log("Appending document fragment to parent (idx: "+document_fragments.indexOf(this)+")")
	// if(arguments[0].nodeType === 11)
	// 	console.log("!!! Appending document fragment to parent (idx: "+document_fragments.indexOf(this)+")")
		
	// if(arguments[0].first_root === undefined)
	// {
	// 	arguments[0].first_root = previous_root;
	// 	// debug_nodes.push(arguments[0]);
	// }

	var was_document_fragment = (child.nodeType === 11);
	var skip_nodes = [];
	if(was_document_fragment)
	{
		// DEBUG
		//console.log("Document fragment (idx: "+document_fragments.indexOf(child)+") gets appended to ", this);		
		
		skip_nodes.concat(child.childNodes);
	}

	// Execute real appendChild
    var return_value = originalAppendChild.apply(this, arguments);

	/* ### ANALYZE APPENDED SUBTREE AFTER DOCUMENT FRAGMENT DISPERSES ### */
	// if(child.nodeType === 11 && child.getRootNode().nodeType !== 11) //  && subtreeRoot !== document)
	var new_root = this.getRootNode();
	// if(previous_root !== new_root && new_root === document)
	// if(was_document_fragment && new_root === document) //  && previous_root !== document)
	if(new_root !== previous_root )//&& new_root === document)
	{
		console.log("previous root: ", previous_root, "\nnew root: ", new_root, "\nnew parent: ", this);
		for(var i = 0, n = this.childNodes.length; i < n; i++)
		{
			var node = this.childNodes[i];
			if(node in skip_nodes)
				continue;
		
			AnalyzeNode(node);
			// Also, check its whole subtree
			ForEveryChild(node, AnalyzeNode);
		}
	}

	return return_value;

};

var document_fragments = [];

Document.prototype.createDocumentFragment = function() {
	var fragment = new DocumentFragment();

	// DEBUG
	// console.log("Document fragment got created, access with index "+document_fragments.length+" in array document fragments.");

	document_fragments.push(fragment);

	window.observer.observe(fragment, window.observer_config);

	return fragment;
};

// DEBUGGING
var analyzed_nodes = [], not_analyzed_nodes = [];
var not_analyzed_roots = new Set();
function CountAnalyzedNodes(root=document.documentElement, print_nodes=false){
	var count = 0;
	analyzed_nodes = [];
	not_analyzed_nodes = [];
	times_analyzed_histogram = [];
	ForEveryChild(root, (n) => {
		count++;
		if(n.analyzed)
		{
			analyzed_nodes.push(n);
			if(times_analyzed_histogram[n.times_analyzed] === undefined)
				times_analyzed_histogram[n.times_analyzed] = 1;
			else
				times_analyzed_histogram[n.times_analyzed]++;
		}
		else
		{
			not_analyzed_nodes.push(n);
			if(n.parentNode && n.parentNode.analyzed)
			{
				not_analyzed_roots.add(n.parentNode);
			}
			if(print_nodes)
				console.log(n);
		}
	});
	times_analyzed_histogram[0] = not_analyzed_nodes.length;

	if(root === document.documentElement)
	{
		ConsolePrint("Found "+analyzed_nodes.length+" analyzed and "+not_analyzed_nodes.length+" not analyzed of "+count+" nodes"
			+" | percentage not analyzed: "+not_analyzed_nodes.length/count*100+"%");
		ConsolePrint("Found "+not_analyzed_roots.size+" not analyzed root nodes.");
	}
	return {"analyzed" : analyzed_nodes, 
		"not_analyzed" : not_analyzed_nodes, 
		"not_analyzed_roots" : not_analyzed_roots,
		"times_analyzed_histogram" : times_analyzed_histogram};
}

function AnalyzeNode(node)
{
	// DEBUG
	if(!node.analyzed)
	{
		node.analyzed = true;
		node.times_analyzed = 0;
	}
	node.times_analyzed++;
	

	//if( (node.nodeType == 1 || node.nodeType > 8) && (node.hasAttribute && !node.hasAttribute("nodeType")) && (node !== window)) // 1 == ELEMENT_NODE
	if(typeof(node.hasAttribute) === "function")
	{
		// /* ### HANDLING APPENDED### */
		// // If node is possible DocFrag subtree root node, remove it from set and analyze its subtree too
		// if(window.appendedSubtreeRoots.delete(node))
		// {
		// 	// console.log("Found document fragment child, whose subtree still has to be analyzed. Performing now.");
		// 	ForEveryChild(node, AnalyzeNode);
		// }

		var computedStyle = window.getComputedStyle(node, null);
		if(!computedStyle)
		{
			console.log("AnalyzeNode: computedStyle is null.");
		}

		// Identify fixed elements on appending them to DOM tree
		if(computedStyle && computedStyle.getPropertyValue('position') == 'fixed') 
		{
			// Returns true if new FixedElement was added; false if already linked to FixedElement Object
			if(AddFixedElement(node))
				UpdateChildNodesRects(node.parent);
				// UpdateDOMRects("AnalyzeNode -- AddFixedElement "+node.className);
		}

		if(node.tagName == "DIV")
		{
			var role = node.getAttribute('role');
			if(role)
			{
				role = role.toLowerCase();
				if(role == "combobox" || role == "textbox")
				{
					CreateDOMTextInput(node);
				}
			}
		}

		if(node.tagName === "IFRAME")
		{

			try
			{
				if(node.contentDocument && node.contentDocument.children.length > 0)
				{
					console.log("iframe added, now also observing:", node.contentDocument.children[0]);
					window.observer.observe(node.contentDocument.children[0], window.observer_config);
				}
			}
			catch(e)
			{
				console.log("Caught exception: ", e);
			}
		}

		if(node.tagName === "VIDEO")
		{
			CreateDOMVideo(node);
		}
		if(node.tagName === "SELECT")
		{
			CreateDOMSelectField(node);
		}

		// Fix for Facebook's in-site messenger
		// --> Added wrong elements as text inputs?!
		// if(node.tagName == "DIV" && node.hasAttribute("data-offset-key"))
		// {
		// 	CreateDOMTextInput(node);
		// 	console.log("Found possible text area on Facebook? ", node);
		// }

		// Find high res favicons
		// TODO: Loading of smaller resolutions could be prevented
		if(node.tagName == "LINK")
		{
			if(node.rel && node.rel.includes("apple-touch-icon"))
			{
				SendFaviconURLtoCEF(node.href);
			}
			if(node.href && node.href.includes("favicon") && node.href.includes(".png"))
			{
				SendFaviconURLtoCEF(node.href);
			}
		}
		if(node.tagName == "META")
		{
			if(prop = node.getAttribute("property"))
			{
				if(prop.includes("og:image"))
				{
					SendFaviconURLtoCEF(node.content);
				}
			}
			if(name = node.getAttribute("name"))
			{
				if(name.includes("msapplication-TileImage"))
				{
					SendFaviconURLtoCEF(node.content);
				}
			}
			
		}

		// Find text links
		if(node.tagName == 'A' )
		{
			CreateDOMLink(node);
		}

        // TODO: it appears to me that button always will create a button, so should be classified as DOMLink. And "none" type for input is text input
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

			if(node.type === "checkbox")
			{
				CreateDOMCheckbox(node);
			}
		}
		// textareas or DIVs, who are treated as text fields
		if(node.tagName == 'TEXTAREA')
		{
			CreateDOMTextInput(node);
		}

		// Buttons
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
		if(computedStyle && (node.tagName === "DIV" || node.tagName === "P"))// && rect.width > 0 && rect.height > 0)
		{
			var overflow = computedStyle.getPropertyValue("overflow");
			var valid_overflow = ["scroll", "auto", "hidden"];
			if(valid_overflow.indexOf(overflow) !== -1)
			{
				CreateDOMOverflowElement(node);
			}
			else
			{
				var overflowX = computedStyle.getPropertyValue("overflow-x");
				var overflowY = computedStyle.getPropertyValue("overflow-y");
				if(valid_overflow.indexOf(overflowX) !== -1 || valid_overflow.indexOf(overflowY) !== -1)
				//if(overflowX === "auto" || overflowX === "scroll" || overflowY === "auto" || overflowY === "scroll")
				{
					CreateDOMOverflowElement(node);
				}
			}
		}


		// Update whole <form> if transition happens in form's subtree
		// (For shifting elements in form (e.g. Google Login) )
		// if(node.tagName == 'FORM')
		// {

		// NOTE: Do this for every node, not only forms
		node.addEventListener('webkitTransitionEnd', function(event){
				console.log("Recognized webkitTransitionEnd for node: ", node);

				ForEveryChild(node, function(child){
					if(!child.analyzed)
						AnalyzeNode(child);
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

window.debug = false;

// Instantiate and initialize the MutationObserver
function MutationObserverInit()
{ 
	ConsolePrint('Initializing Mutation Observer ... ');

	window.observer = new MutationObserver(
		function(mutations) 
		{
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
			  			var attr; // attribute name of attribute which has changed


		  				if(node.nodeType == 1) // 1 == ELEMENT_NODE
		  				{
		  					attr = mutation.attributeName;

							// ### FIXED HIERARCHY HANDLING ###
							// Created FixedElement adds attribute childFixedId with its id as value to all
							// its direct child nodes. Rest of the tree cascade down by using MutationObserver
							// as follows:
							// 1.) Set equivalently valued childFixedId attribute in each direct child node
							// 2.) Check if corresponding domObj exists and add fixObj with childFixedId as id
							if(attr === "childfixedid")
							{
								var id = node.getAttribute(attr);

								// Set childFixedId for each child of altered node

								node.childNodes.forEach((child) => {
									// Extend node by given attribute
									if(typeof(child.setAttribute) === "function")
									{
										child.setAttribute("childFixedId", id);
									}
								});
								
								// Update fixObj in DOMObject
								var fixObj = GetFixedElementById(id);
								var domObj = GetCorrespondingDOMObject(node);
								if(domObj !== undefined)
								{
									domObj.setFixObj(fixObj);	// fixObj may be undefined
									domObj.updateRects();
								}
							}

							if(attr === "scrollWidth" || attr === "scrollHeight")
								console.log("### Detected changes in attr "+attr+" ###");

							// ### OVERFLOW HIERARCHY HANDLING ###
							// node's overflowid attribute was set (changed): Set childrens' overflowIds when node gets tagged as part of overflow subtree 
							if(attr == "overflowid")
							{
								// console.log(attr, "changed for ", node);
								var id = node.getAttribute("overflowid");
								var domObj = SetOverflowObjectViaId(node, id);
								if(domObj === undefined)
									var skip_subtree = false;
								else 
									var skip_subtree = (domObj.Class === "DOMOverflowElement");

								if(!skip_subtree)
								{
									for(var i = 0, n = node.childNodes.length; i < n; i++)
									{			
										SetOverflowId(node.childNodes[i], id);
									}
								}
							}


							
							// ### CREATION AND REMOVAL OF FIXED ELEMENTS ###
		  					if(attr == 'style') // ||  (document.readyState != 'loading' && attr == 'class') )
		  					{
								if(window.getComputedStyle(node, null).getPropertyValue('position') === 'fixed')
								{
									if(AddFixedElement(node))
										// Update every Rect, just in case anything changed due to an additional fixed element
										// UpdateDOMRects("AddFixedElement");
										UpdateChildNodesRects(node.parent);
								}
								else 
								{
									// Checks if node corresponds to fixedObj and removes it, when true
									RemoveFixedElement(node);
								}

								var curr_display_none = (node.style.display === "none");
								var old_display_none = (mutation.oldValue !== null && typeof(mutation.oldValue.contains)  === "function") ? 
									mutation.oldValue.contains("display: none") : false;
									
								if((curr_display_none && !old_display_none) || (!curr_display_none && old_display_none))
								{
									console.log("style.display changed: curr="+curr_display_none+", old="+old_display_none);
									ForEveryChild(node, (c) => { UpdateNodesRect(c); })
								}

							}


							if(attr == "id")
							{
								var domObj = GetCorrespondingDOMObject(node);
								if(domObj !== undefined && domObj.Class === "DOMTextInput")
									SendAttributeChangesToCEF("HTMLId", domObj);
							}

						
							if(attr == 'class')
							{	
								var domObj = GetCorrespondingDOMObject(node);
								if(domObj !== undefined && domObj.Class === "DOMTextInput")
									SendAttributeChangesToCEF("HTMLClass", domObj);
							
								// Node might get fixed or un-fixed if class changes
								if(typeof(node.getAttribute) === "function" && 
									window.getComputedStyle(node, null).getPropertyValue("position") === "fixed")
								{
									AddFixedElement(node);
								}
								// TODO: Remove corresponding fixed element if node gets unfixed

								// Update (if existant) DOM object's rects if node's class changed
								// var domObj = GetCorrespondingDOMObject(node);
								if(domObj !== undefined)
									domObj.updateRects();

								// ### FIXED ELEMENT BOUNDING BOX UPDATES ###
								// Changes in attribute 'class' may indicate that a fixed element's union of bounding rects needs to be updated
								// Trigger fixed parent update, if it exists. All of its children will be updated
								var childFixedId = node.getAttribute("childFixedId");
								if(childFixedId !== null && childFixedId !== null)
								{
									var fixObj = GetFixedElementById(childFixedId);
									if(fixObj !== undefined)
									{
										fixObj.updateRects();
										return;
									}
								}

								// Else, if node is fixed, subtree will be updated too
								var fixObj = GetFixedElementByNode(node);
								if(fixObj !== undefined)
								{
									fixObj.updateRects();
									return;
								}

								// Update subtree if class changes
								ForEveryChild(node, (child) => { UpdateNodesRect(child); });
							
							}

							if(attr == "style")
							{
								// Goal: Recognise changes in e.g. style.display
								// 'solution': Trigger rect update if changes in style took place. Direct change in style would be
								// value assignment, which will be recognised in MutationObserver
								UpdateNodesRect(node);
								ForEveryChild(node, (child) => { UpdateNodesRect(child); });
								// TODO: Changes in style may occure when scrolling some elements ... might be a lot of Rect Update calls!
		  					} // END attr == 'style'

		  				} // END node.nodeType == 1
		  			} // END mutation.type == 'attributes'





		  			if(mutation.type === 'childList') // TODO: Called upon each appending of a child node??
		  			{
						// console.log("type: ", mutation.type,"\ttarget: ", mutation.target, "\tnodeType: ", mutation.target.nodeType);

		  				// Check if fixed nodes have been added as child nodes
			  			var nodes = mutation.addedNodes;
						var parent = mutation.target;
						
						// Handle every appended child node
			  			nodes.forEach((node) => {
							if(node === undefined)
								return;

							// Child was appended AND triggers childlist mutation, as expected!
							// Fix for those, which don't: Analyze them with CefPoll
							appended_nodes.delete(node);

							// // Never executed!
							// if(node.nodeType === 11)
							// {
							// 	console.log("Realized that document fragment was appended to DOM tree!");
							// }
							// // TODO: This is already done in appendChild method!
							// // Mark child nodes of DocumentFragment, in order to being able to analyze their subtrees later
							// if(mutation.target.nodeType === 11)
							// {
							// 	// Add node as possible root, which might not be recognized when DocFrag disappears
							// 	window.appendedSubtreeRoots.add(node);
							// 	// Remove knowledge about every possible root due to a DocFrag in whole subtree
							// 	ForEveryChild(node, function(child){ window.appendedSubtreeRoots.delete(child); });
							// }

							// DEBUG
							// console.log("childList mutation: ", node);							
							
							// Set children (and their subtree) to fixed, if parent is also fixed
							if(parent !== undefined && parent.nodeType === 1 && node.nodeType === 1)
							{
								var id = parent.getAttribute("childfixedid");
								if(id === null || id === undefined)
									id = parent.getAttribute("fixedid");

								if(id !== null && id !== undefined)
								{
									node.setAttribute("childfixedid", id);
									var domObj = GetCorrespondingDOMObject(node);
									if(domObj !== undefined)
										domObj.setFixObj(GetFixedElementById(id));
								}
							}

							// If parent is contained in overflow element, then child will also be
							if(parent !== undefined && typeof(parent.getAttribute) === "function") 
							{
								var parentDomObj = GetCorrespondingDOMObject(parent);
								
								// If parent is overflow element, use its id instead
								if(parentDomObj !== undefined && parentDomObj.Class === "DOMOverflowElement")
									var overflowId = parentDomObj.getId();
								else
									var overflowId = parent.getAttribute("overflowid");
								
								SetOverflowId(node, overflowId);

							}
						
			  				AnalyzeNode(node);
						}); // END of forEach

						// Update rects of fixed parent's subtree after having appended child nodes 
						// TODO: Partial rect update for fixed elements?
						if(parent.nodeType === 1)
						{
							var fixId = parent.getAttribute("childFixedId");
							if(fixId !== null && fixId !== undefined)
							{
								var fixObj = GetFixedElementById(fixId);
								if(fixObj !== null && fixObj !== undefined)
								{
									fixObj.updateRects();
								}
							}
						}

						

			  			
			  			mutation.removedNodes.forEach((node) => {
			  				if(node !== undefined && node.nodeType === 1)
			  				{
								RemoveFixedElement(node, false);
								
								/* Don't remove Overflow if only a child was removed!
								var overflowId = node.getAttribute("overflowId");
								if(overflowId !== null)
								{
									console.log("Nope. ", overflowId, node);
									// RemoveDOMOverflowElement(overflowId);
								}
								*/	
			  				}

							UpdateNodesRect(node);
							// Trigger Rect Updates after removal of (several) node(s)
							ForEveryChild(node, (child) => { UpdateNodesRect(child); });
						});


		  			} // END mutation.type == 'childList'

					mutation_observer_working_time += (Date.now() - working_time_start);

						
		  		} // END forEach mutation


			 );    
			 
			 // Empty records
			 window.observer.takeRecords();
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
	window.observer_config = { attributes: true, childList: true, characterData: true, subtree: true, characterDataOldValue: false, attributeOldValue: true};

	// eigentliche Observierung starten und Zielnode und Konfiguration übergeben
	window.observer.observe(window.document, observer_config);

	ConsolePrint('MutationObserver was told what to observe.');

	// TODO: Tweak MutationObserver by defining a more specific configuration



	
} // END OF MutationObserverInit()


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

function CreateCustomMutationObserver(node, attrFunc, childFunc)
{
	var observer = new MutationObserver(
		function(mutations) {
		  	mutations.forEach(
		  		function(mutation){
					if(mutation.type === 'attributes')
						attrFunc(mutation);
					if(mutation.type == 'childList')
						childFunc(mutation);

				  }
			);
		}
	);
	var config = {
		attributes: true, childList: true, characterData: true, 
		subtree: true, characterDataOldValue: true, attributeOldValue: true
	};
	observer.observe(node, config);
	return observer;
}


ConsolePrint("Successfully imported dom_mutationobserver.js!");


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

