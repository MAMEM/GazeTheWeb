//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action for text selection. Utilizes zoom coordinate action to determine
// end of text selection through being subclass of that action. Performs selection
// of text within webpage.
// - Input: vec2 coordinate in WebViewPixel space as start point of text selection
// - Output: vec2 coordinate in WebViewPixel space as end point of text selection

#ifndef TEXTSELECTIONACTION_H_
#define TEXTSELECTIONACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/ZoomCoordinateAction.h"
#include "src/Utils/LerpValue.h"

class TextSelectionAction : public ZoomCoordinateAction
{
public:

    // Constructor
	TextSelectionAction(TabInteractionInterface* pTab);

    // Update retuns whether finished with execution
    virtual bool Update(float tpf, TabInput tabInput);

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

protected:

};

#endif // TEXTSELECTIONACTION_H_
