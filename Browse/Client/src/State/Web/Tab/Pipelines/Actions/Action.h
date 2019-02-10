//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Abstract superclass of all actions. Action is atomic interaction of user with
// current tab.

#ifndef ACTION_H_
#define ACTION_H_

#include "src/Input/Input.h"
#include "src/State/Web/Tab/Pipelines/Actions/ActionDataMap.h"
#include "src/Input/VoiceInput.h"

// Forward declaration
class TabInteractionInterface;

class Action
{
public:

    // Constructor
    Action(TabInteractionInterface* pTab);

    // Destructor
    virtual ~Action() = 0;

    // Update retuns whether finished with execution
    virtual bool Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput) = 0;

    // Draw
    virtual void Draw() const = 0;

    // Activate
    virtual void Activate() = 0;

    // Deactivate
    virtual void Deactivate() = 0;

    // Abort
    virtual void Abort() = 0;

	// Set input data value
	template <typename T>
	void SetInputValue(std::string type, T value) { _inputData.SetValue(type, value); }

    // Get output data value in reference and returns, whether value was filled
	template <typename T>
	bool GetOutputValue(std::string type, T& rValue) const { return _outputData.GetValue(type, rValue); }

protected:

    // Add slot to data maps with default values
	void AddIntInputSlot		(std::string type)	{ _inputData.AddIntSlot(type); }
	void AddInt64InputSlot		(std::string type)	{ _inputData.AddInt64Slot(type); }
	void AddFloatInputSlot		(std::string type)	{ _inputData.AddFloatSlot(type); }
    void AddVec2InputSlot		(std::string type)	{ _inputData.AddVec2Slot(type); }
    void AddStringInputSlot		(std::string type)	{ _inputData.AddStringSlot(type); }
	void AddString16InputSlot	(std::string type)	{ _inputData.AddString16Slot(type); }

	void AddIntOutputSlot		(std::string type)	{ _outputData.AddIntSlot(type); }
    void AddInt64OutputSlot		(std::string type)	{ _outputData.AddInt64Slot(type); }
    void AddFloatOutputSlot		(std::string type)	{ _outputData.AddFloatSlot(type); }
    void AddVec2OutputSlot		(std::string type)	{ _outputData.AddVec2Slot(type); }
    void AddStringOutputSlot	(std::string type)	{ _outputData.AddStringSlot(type); }
    void AddString16OutputSlot	(std::string type)	{ _outputData.AddString16Slot(type); }

	// Add slot to data maps with custom values
	void AddIntInputSlot		(std::string type, int value)				{ _inputData.AddIntSlot(type, value); }
	void AddInt64InputSlot		(std::string type, int64 value)				{ _inputData.AddInt64Slot(type, value); }
	void AddFloatInputSlot		(std::string type, float value)				{ _inputData.AddFloatSlot(type, value); }
	void AddVec2InputSlot		(std::string type, glm::vec2 value)			{ _inputData.AddVec2Slot(type, value); }
	void AddStringInputSlot		(std::string type, std::string value)		{ _inputData.AddStringSlot(type, value); }
	void AddString16InputSlot	(std::string type, std::u16string value)	{ _inputData.AddString16Slot(type, value); }

	void AddIntOutputSlot		(std::string type, int value)				{ _outputData.AddIntSlot(type, value); }
	void AddInt64OutputSlot		(std::string type, int64 value)				{ _outputData.AddInt64Slot(type, value); }
	void AddFloatOutputSlot		(std::string type, float value)				{ _outputData.AddFloatSlot(type, value); }
	void AddVec2OutputSlot		(std::string type, glm::vec2 value)			{ _outputData.AddVec2Slot(type, value); }
	void AddStringOutputSlot	(std::string type, std::string value)		{ _outputData.AddStringSlot(type, value); }
	void AddString16OutputSlot	(std::string type, std::u16string value)	{ _outputData.AddString16Slot(type, value); }

    // Get input data value in reference and returns, whether value was filled
	template <typename T>
	bool GetInputValue(std::string type, T& rValue) const { return _inputData.GetValue(type, rValue); }

    // Set output data value
	template <typename T>
	void SetOutputValue(std::string type, T value) { _outputData.SetValue(type, value); }

    // Pointer to interface which enables interaction with tab
    TabInteractionInterface* _pTab;

private:

    // Input data
    ActionDataMap _inputData;

    // Output data
    ActionDataMap _outputData;

};

#endif // ACTION_H_
