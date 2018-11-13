ConsolePrint("Starting to import dom_nodes_interaction.js ...");


// Deprecated. Now done via keyboard key stroke emulation, when focused
// DOMTextInput.prototype.inputText = function(text, submit){


DOMOverflowElement.prototype.scroll = function(gazeX, gazeY, fixedIds){
	// console.log("scroll called with "+gazeX+" "+gazeY+" (ids: "+fixedIds+") -- hidden? "+this.hidden_overflow);

	if(this.hidden_overflow)
		return [0,0];


	// if(fixedIds === undefined || fixedIds.length > 0 && fixedIds.indexOf(this.getFixedId()) === -1)
	// 	return;

	// Do not use cut-off rect and keep scrolling velocity untouched if partially hidden
	var rects = this.getAdjustedClientRects();

	// Update scrolling position according to current gaze coordinates
	// Idea: Only scroll if gaze is somewhere near the overflow elements edges
	if(rects.length > 0)
	{
		var rect = rects[0];

		var width = this.node.clientWidth; // without scrollbar
		var height = this.node.clientHeight;

		// var centerX = rect[1] + Math.round(width / 2);
		// var centerY =  rect[0] + Math.round(height / 2);

		// TODO: Substract scrollbar areas from overflow rect! So, you don't have to look at e.g. the horizontal scrollbar
		// in order to scroll vertically
		var distLeft = gazeX - rect[1];   // negative values imply gaze outside of element
		var distRight = rect[1] + width - gazeX;
		var distTop = gazeY - rect[0];
		var distBottom = rect[0] + height - gazeY;

		// Treshold for actual scrolling taking place, maximum distance to border where scrolling takes place
		var tresholdX = 1 / 2.5 * (width / 2);
		var tresholdY = 1 / 2.5 * (height / 2);

		var maxScrollingPerFrame = 10;
		// Actual scrolling, added afterwards
		var scrollX = 0;
		var scrollY = 0;

		if(distLeft >= 0 && distLeft < tresholdX)
		{
			scrollX -= (maxScrollingPerFrame * ( 1 - (distLeft / tresholdX) ));
		}
		if(distRight >= 0 && distRight < tresholdX)
		{
			scrollX += (maxScrollingPerFrame * ( 1 - (distRight / tresholdX) ));
		}
		if(distTop >= 0 && distTop < tresholdY)
		{
			scrollY -= (maxScrollingPerFrame * (1 - (distTop / tresholdY)));
		}
		if(distBottom >= 0 && distBottom < tresholdY)
		{
			scrollY += (maxScrollingPerFrame * (1 - (distBottom / tresholdY)));
		}

		// Execute scrolling
		this.node.scrollLeft += scrollX;
		this.node.scrollTop += scrollY;

		// DEBUG
		// console.log("scrolling by "+scrollX+" "+scrollY);

		return [scrollX, scrollY];
	}
	return [0,0];
		console.log("scrolling by 0 0");
	
}

DOMVideo.prototype.setPlaying = function(playing)
{
    if (playing)
    {
        this.node.play();
        ConsolePrint("DOMVideo id=" + this.getId() + ": Now playing.");
    }
    else
	{
		this.node.pause();
		ConsolePrint("DOMVideo id="+this.getId()+": Now paused.");
	}
}

DOMVideo.prototype.setMuted = function(muted)
{
	this.node.muted = muted;
	ConsolePrint("DOMVideo id="+this.getId()+": muted="+muted);
}

DOMVideo.prototype.setVolume = function(volume)
{
	this.node.volume = volume;
	ConsolePrint("DOMVideo id="+this.getId()+": Volume is now "+volume);
}

DOMVideo.prototype.jumpToSecond = function(sec)
{
	this.node.currentTime = sec;
	ConsolePrint("DOMVideo id="+this.getId()+": Now playing from second "+sec);
}

DOMVideo.prototype.skipSeconds = function (sec)
{
    this.node.currentTime += sec;
    ConsolePrint("DOMVideo id=" + this.getId() + ": Now playing from second " + sec);
}

DOMVideo.prototype.setFullscreen = function(fullscreen)
{
	if(fullscreen)
	{
		// DO MAGIC INSTEAD
		// this.node.webkitRequestFullscreen();

		var button = document.createElement("button");
		button.style.position = "fixed";
		button.style.top = "0px";
		button.style.left = "0px";
		button.style.width = "100%";
		button.style.height = "100%";
		button.style.zIndex = "99999";
		button.style.opacity = "0.1";
		button.id = "gtw-helper";
		button.className = "gtw-helper";

		var scoped_id = this.getId();
		var scoped_node = this.node;
		// Don't interact with this button anymore after it was clicked the first time
		button.onclick = function(e){
			// this.style.pointerEvents = "none";
			ConsolePrint("DOMVideo id="+scoped_id+": Fullscreen is set to true");

			console.log("Trying to get node to fullscreen: ", scoped_node);
			
			// Request fullscreen for given video
			// YOUTUBE FIX
			if (scoped_node.className === "video-stream html5-main-video"
                && scoped_node.parentElement
				&& scoped_node.parentElement.parentElement
                && scoped_node.parentElement.parentElement.parentElement
                && scoped_node.parentElement.parentElement.parentElement.parentElement
                && scoped_node.parentElement.parentElement.parentElement.parentElement.parentElement
                && scoped_node.parentElement.parentElement.parentElement.parentElement.parentElement.parentElement
                && scoped_node.parentElement.parentElement.parentElement.parentElement.parentElement.parentElement.parentElement
                && scoped_node.parentElement.parentElement.parentElement.parentElement.parentElement.parentElement.parentElement.parentElement)
			{
			    var container = scoped_node.parentElement.parentElement.parentElement.parentElement.parentElement.parentElement.parentElement;
				container.webkitRequestFullscreen();

				console.log("Requested fullscreen for: ", container);
			}
			else
				scoped_node.webkitRequestFullscreen();
			this.style.position = "static";
			this.id += "-finished";
			this.className += "-finished";

			// Remove button from DOM tree again when it's purpose is done
			document.documentElement.removeChild(this);
		}
		
		// Add newly created button to DOM tree
		document.documentElement.appendChild(button);

		// Tell CEF to emulate click on newly created button
		var rect = button.getBoundingClientRect();
		var response = {
			"command" 	: "EmulateMouseClick",
			"x" 		: (rect.left + rect.width / 2.0),
			"y"			: (rect.top + rect.height / 2.0)
		};
		return response;
	}
	else
		document.webkitExitFullscreen();		
	ConsolePrint("DOMVideo id="+this.getId()+": Fullscreen is set to "+fullscreen);
}

DOMVideo.prototype.toggleMuted = function()
{
    this.node.muted = !this.node.muted;
    ConsolePrint("DOMVideo id=" + this.getId() + ": muted=" + this.node.muted);
}

DOMVideo.prototype.togglePlayPause = function()
{
	if (this.node.paused) 
	{
        this.node.play();
        ConsolePrint("DOMVideo id=" + this.getId() + ": Now playing.");
    }
	else 
	{
        this.node.pause();
        ConsolePrint("DOMVideo id=" + this.getId() + ": Now paused.");
    }
}

DOMVideo.prototype.changeVolume = function(delta)
{
    var value = this.node.volume;
    value = Math.min(Math.max(0.0, value + delta), 1.0);
    this.node.volume = value;
}

DOMVideo.prototype.showControls = function(val)
{
	console.log("DOMVideo.showControls has to be implemented! Called with ", arguments);
}



DOMCheckbox.prototype.setChecked = function(state)
{
	this.node.checked = state;
	console.log("DOMCheckbox, "+this.getId()+": Setting to checked to '"+checked+"'.");
}

ConsolePrint("Successfully imported dom_nodes_interaction.js!");