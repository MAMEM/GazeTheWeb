//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "ClickPipeline.h"
#include "src/State/Web/Tab/Pipelines/Actions/CoordinateActions/MagnificationCoordinateAction.h"
#include "src/State/Web/Tab/Pipelines/Actions/LinkNavigationAction.h"

ClickPipeline::ClickPipeline(TabInteractionInterface* pTab) : Pipeline(pTab)
{
    // Push back magnification coordinate action
	std::shared_ptr<MagnificationCoordinateAction> spMagnificationCoordinateAction = std::make_shared<MagnificationCoordinateAction>(_pTab);
	_actions.push_back(spMagnificationCoordinateAction);

	// Push back link navigation action
	std::shared_ptr<LinkNavigationAction> spLinkNavigationAction = std::make_shared<LinkNavigationAction>(_pTab);
	_actions.push_back(spLinkNavigationAction);

    // Connect actions
    std::unique_ptr<ActionConnector> upConnector =
        std::unique_ptr<ActionConnector>(new ActionConnector(spMagnificationCoordinateAction, spLinkNavigationAction));
    upConnector->ConnectVec2("coordinate", "coordinate");
    _connectors.push_back(std::move(upConnector));
}
