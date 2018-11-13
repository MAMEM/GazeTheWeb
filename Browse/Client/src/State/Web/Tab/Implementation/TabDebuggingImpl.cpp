//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/State/Web/Tab/Tab.h"
#include "src/Master/Master.h"
#include "src/Setup.h"
#include "src/Utils/PrimitiveRenderItem.h"
#include "src/Utils/Helper.h"
#include "src/Utils/Logger.h"
#include "submodules/glm/glm/gtc/matrix_transform.hpp"
// Integrating reduce..
#include <algorithm>
#include <numeric>

// Shader programs (For debugging rectangles)
const std::string vertexShaderSource =
"#version 330 core\n"
"in vec3 posAttr;\n"
"in vec2 uvAttr;\n"
"out vec2 uv;\n"
"uniform mat4 matrix;\n"
"void main() {\n"
"   uv = uvAttr;\n"
"   gl_Position = matrix * vec4(posAttr, 1);\n"
"}\n";

const std::string fragmentShaderSource =
"#version 330 core\n"
"in vec2 uv;\n"
"out vec4 fragColor;\n"
"uniform vec3 color;\n"
"void main() {\n"
"   fragColor = vec4(color,1);\n"
"}\n";

void Tab::InitDebuggingOverlay()
{
	// Line quad
	_upDebugLineQuad = std::unique_ptr<RenderItem>(
		new PrimitiveRenderItem(
			vertexShaderSource,
			fragmentShaderSource,
			Primitive::Type::QUAD_LINES_WITH_DIAGONAL));

	// Fill quad
	_upDebugFillQuad = std::unique_ptr<RenderItem>(
		new PrimitiveRenderItem(
			vertexShaderSource,
			fragmentShaderSource,
			Primitive::Type::QUAD_TRIANGLES));

	// Line
	_upDebugLine = std::unique_ptr<RenderItem>(
		new PrimitiveRenderItem(
			vertexShaderSource,
			fragmentShaderSource,
			Primitive::Type::LINE));
}

void Tab::DrawDebuggingOverlay() const
{
	// Reserve variables
	glm::mat4 model, matrix;

	// Projection
	glm::mat4 projection = glm::ortho(0, 1, 0, 1);

	// Do only draw following when no pipeline is active
	if (!_pipelineActive)
	{
		// Define render function (should not be defined each call)
		const std::function<void(Rect, bool)> renderRect = [&](Rect rect, bool fixed)
		{
			// Fixed or not
			if (!fixed)
			{
				// Subtract scrolling while coordinates are in CEFPixel space
				rect.left -= _scrollingOffsetX;
				rect.right -= _scrollingOffsetX;
				rect.bottom -= _scrollingOffsetY;
				rect.top -= _scrollingOffsetY;
			}

			// Scale from CEFPixel space to WebViewPixel
			rect.left = (rect.left / (float)_upWebView->GetResolutionX()) * (float)_upWebView->GetWidth();
			rect.right = (rect.right / (float)_upWebView->GetResolutionX()) * (float)_upWebView->GetWidth();
			rect.bottom = (rect.bottom / (float)_upWebView->GetResolutionY()) * (float)_upWebView->GetHeight();
			rect.top = (rect.top / (float)_upWebView->GetResolutionY()) * (float)_upWebView->GetHeight();

			// Calculate model matrix
			model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(1.f / _pMaster->GetWindowWidth(), 1.f / _pMaster->GetWindowHeight(), 1.f));
			model = glm::translate(model, glm::vec3(_upWebView->GetX() + rect.left, _upWebView->GetHeight() - (rect.bottom), 1));
			model = glm::scale(model, glm::vec3(rect.Width(), rect.Height(), 0));

			// Combine matrics
			matrix = projection * model;

			// Fill uniform with matrix (no need for Bind() since bound in called context)
			_upDebugLineQuad->GetShader()->UpdateValue("matrix", matrix);

			// Render rectangle
			_upDebugLineQuad->Draw(GL_LINES);
		};

		// Bind debug render item for line quad
		_upDebugLineQuad->Bind();

		// ### DOMTRIGGER ###

		// Set rendering up for DOMTrigger
		_upDebugLineQuad->GetShader()->UpdateValue("color", DOM_TRIGGER_DEBUG_COLOR);

		// TODO: also implement for select fields

		// Go over all TextInputTriggers
		for (const auto& rNodeTriggerPair : _textInputTriggers)
		{
			const auto& rDOMTrigger = rNodeTriggerPair.second;

			// Render rects
			for (const auto rRect : rDOMTrigger->GetDOMRects())
			{
				/*
				if (!rDOMTrigger->GetDOMIsPasswordField()) // TODO: what is this? Different color for password fields?
				{
					renderRect(rRect, rDOMTrigger->GetDOMFixed());
					//LogDebug("TabDebugImpl: Is trigger fixed? ", rDOMTrigger->GetDOMFixed());
				}
				else */
				{
					_upDebugLineQuad->GetShader()->UpdateValue("color", glm::vec3(0.0f, 1.f, 1.f));
					renderRect(rRect, rDOMTrigger->GetDOMFixed());
				
					_upDebugLineQuad->GetShader()->UpdateValue("color", DOM_TRIGGER_DEBUG_COLOR);
				}
			}
		}


		// ### DOMTEXTLINKS ###

		// Set rendering up for DOMTextLink
		_upDebugLineQuad->GetShader()->UpdateValue("color", DOM_TEXT_LINKS_DEBUG_COLOR);

		// Go over all DOMTextLinks
		for (const auto& rIdNodePair : _TextLinkMap)
		{
			const auto& rDOMTextLink = rIdNodePair.second;
			// Render rects
			for (const auto rRect : rDOMTextLink->GetRects())
			{
				renderRect(rRect, (rDOMTextLink->IsFixed()));
				bool visible = !rDOMTextLink->IsOccluded();
				if (visible)
					renderRect(
						rRect,
						(rDOMTextLink->IsFixed())
					);
				else
				{
					_upDebugLineQuad->GetShader()->UpdateValue("color", glm::vec3(0.2, 0.2, 0.2));
					renderRect(rRect, rDOMTextLink->IsFixed());
					_upDebugLineQuad->GetShader()->UpdateValue("color", DOM_TEXT_LINKS_DEBUG_COLOR);
				}
				// else
				//	LogInfo("TabDebuggingImpl: Hiding DOMTextLink with id=", rDOMTextLink->GetId());
			}
		}

		// DEBUG - links containing line break are shown in another color
		_upDebugLineQuad->GetShader()->UpdateValue("color", glm::vec3(1.f, 0.f, 1.f));
		for (const auto& rIdNodePair : _TextLinkMap)
		{
			const auto& rDOMTextLink = rIdNodePair.second;
			if (rDOMTextLink->GetRects().size() > 1)
				renderRect(
					rDOMTextLink->GetRects()[1], 
					(rDOMTextLink->IsFixed())
				);
		}

		// ### SELECT FIELDS ###
		// Set rendering up for DOMSelectFields
		_upDebugLineQuad->GetShader()->UpdateValue("color", DOM_SELECT_FIELD_DEBUG_COLOR);
		for (const auto& rIdNodePair : _SelectFieldMap)
		{
			const auto& rDOMSelectField = rIdNodePair.second;
			// Render rects
			for (const auto rRect : rDOMSelectField->GetRects())
			{
				renderRect(
					rRect, 
					(rDOMSelectField->IsFixed())
				);
			}
		}

		// ### FIXED ELEMENTS ###

		// Set rendering up for fixed element
		_upDebugLineQuad->GetShader()->UpdateValue("color", FIXED_ELEMENT_DEBUG_COLOR);

		// Go over all fixed elements vectors
		for (const auto& rFixedElements : _fixedElements)
		{
			// Go over fixed elements
			for (const auto& rFixedElement : rFixedElements)
			{
				// Render rect
				if (rFixedElement.Height() > 0 && rFixedElement.Width() > 0)
					renderRect(rFixedElement, true);
			}
		}

		// ### OVERFLOW ELEMENTS ###
		_upDebugLineQuad->GetShader()->UpdateValue("color", glm::vec3(255.f / 255.f, 127.f / 255.f, 35.f / 255.f));

		for (const auto& rIdNodePair : _OverflowElementMap)
		{
			const auto& rOverflowElement = rIdNodePair.second;
			for (const auto& rect : rOverflowElement->GetRects())
			{
				renderRect(
					rect,
					(rOverflowElement->IsFixed())
				);
			}
		}

		// ### DOM VIDEO ELEMENTS ### 
		_upDebugLineQuad->GetShader()->UpdateValue("color", glm::vec3(255.f / 255.f, 255.f / 255.f, 60.f / 255.f));

		for (const auto& rIdNodePair : _VideoMap)
		{
			const auto& rVideoNode = rIdNodePair.second;
			for (const auto& rect : rVideoNode->GetRects())
			{
				renderRect(
					rect,
					(rVideoNode->IsFixed())
				);
			}
		}

		// ### DOM CHECKBOX ELEMENTS ### 
		_upDebugLineQuad->GetShader()->UpdateValue("color", glm::vec3(120.f / 255.f, 0.f / 255.f, 255.f / 255.f));

		for (const auto& rIdNodePair : _CheckboxMap)
		{
			const auto& rCheckboxNode = rIdNodePair.second;
			for (const auto& rect : rCheckboxNode->GetRects())
			{
				renderRect(
					rect,
					(rCheckboxNode->IsFixed())
				);
			}
		}
	}

	// ### EXACT GAZE INPUT VISUALIZTION ###

	/*

	// Bind render item and set color
	_upDebugFillQuad->Bind();
	_upDebugFillQuad->GetShader()->UpdateValue("color", glm::vec3(255.f / 255.f, 127.f / 255.f, 35.f / 255.f));

	// Do it for each gaze sample
	for (const glm::vec2& rGaze : _gazeDebuggingQueue)
	{
		// Calculate model matrix
		model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(1.f / _pMaster->GetWindowWidth(), 1.f / _pMaster->GetWindowHeight(), 1.f));
		model = glm::translate(model, glm::vec3(rGaze.x, _pMaster->GetWindowHeight() - rGaze.y, 1)); // TODO: breaks when web view is not filling complete height
		model = glm::scale(model, glm::vec3(2, 2, 0));

		// Combine matrics
		matrix = projection * model;

		// Fill uniform with matrix
		_upDebugFillQuad->GetShader()->UpdateValue("matrix", matrix);

		// Render rectangle
		_upDebugFillQuad->Draw(GL_TRIANGLES);
	}

	*/
}

void Tab::Debug_DrawRectangle(glm::vec2 coordinate, glm::vec2 size, glm::vec3 color) const
{
	// Bind render item and set color
	_upDebugFillQuad->Bind();
	_upDebugFillQuad->GetShader()->UpdateValue("color", color);

	// Projection
	glm::mat4 projection = glm::ortho(0, 1, 0, 1);

	// Calculate model matrix
	auto model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(1.f / _pMaster->GetWindowWidth(), 1.f / _pMaster->GetWindowHeight(), 1.f));
	model = glm::translate(model, 
		glm::vec3(
			glm::vec2(coordinate.x - (size.x / 2.f), _pMaster->GetWindowHeight() - coordinate.y - (size.y / 2.f)) // coordinate in center of rectangular in correct system TODO: breaks when web view is not filling complete height
			+ glm::vec2(_upWebView->GetX(), _upWebView->GetY()), 1)); // offset of web view
	model = glm::scale(model, glm::vec3(size, 0.f));

	// Combine matrics
	auto matrix = projection * model;

	// Fill uniform with matrix
	_upDebugFillQuad->GetShader()->UpdateValue("matrix", matrix);

	// Render rectangle
	_upDebugFillQuad->Draw(GL_TRIANGLES);
}

void Tab::Debug_DrawLine(glm::vec2 originCoordinate, glm::vec2 targetCoordinate, glm::vec3 color) const
{
	// Bind render item and set color
	_upDebugLine->Bind();
	_upDebugLine->GetShader()->UpdateValue("color", color);

	// Projection
	glm::mat4 projection = glm::ortho(0, 1, 0, 1);

	// Calculate model matrix
	auto model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(1.f / _pMaster->GetWindowWidth(), 1.f / _pMaster->GetWindowHeight(), 1.f));
	model = glm::translate(model,
		glm::vec3(
			glm::vec2(originCoordinate.x, _pMaster->GetWindowHeight() - (originCoordinate.y)) // coordinate in correct system TODO: breaks when web view is not filling complete height
			+ glm::vec2(_upWebView->GetX(), _upWebView->GetY()), 1)); // offset of web view
    model = glm::rotate(model, (float)atan2(-(targetCoordinate.y - originCoordinate.y), targetCoordinate.x - originCoordinate.x), glm::vec3(0, 0, 1)); // atan2 takes first y then x TODO: breaks when web view is not filling complete height
	model = glm::scale(model, glm::vec3(glm::distance(originCoordinate, targetCoordinate), 0.f, 0.f));

	// Combine matrics
	auto matrix = projection * model;

	// Fill uniform with matrix
	_upDebugLine->GetShader()->UpdateValue("matrix", matrix);

	// Render line
	_upDebugLine->Draw(GL_LINES);
}
