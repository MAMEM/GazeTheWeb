//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
//============================================================================


ConsolePrint("Starting to import dom_nodes_helpers.js ...");

DOMNode.prototype.setCppReady = function(){
    this.cppReady = true;
}
DOMNode.prototype.isCppReady = function(){
    return this.cppReady;
}
DOMNode.prototype.getAdjustedClientRects = function(altNode){
    var rects = (altNode === undefined) ? this.node.getClientRects() : altNode.getClientRects();
    if (this.getFixedId() === -1)
    {
        return rects.map(
            (rect) => {
                return AdjustRectCoordinatesToWindow(rect);
            });
    }
    // If node is fixed, return unaltered rect
    return rects.map((r) => {return [r.top, r.left, r.bottom, r.right];});
}


ConsolePrint("Successfully imported dom_nodes_helpers.js!");