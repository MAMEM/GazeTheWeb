//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel M�ller (muellerd@uni-koblenz.de)
//============================================================================
// Class for DOM nodes which are used by Tab.

#ifndef DOMNODE_H_
#define DOMNODE_H_

#include "src/Typedefs.h"
#include "src/Utils/Rect.h"
#include "src/Utils/glmWrapper.h"
#include "src/State/Web/Tab/DOMNodeType.h"
#include <vector>
#include <string>

class DOMNode
{
    public:
        DOMNodeType GetType() const { return _type; }
        int64 GetFrameID() const { return _frameID; }
        int GetNodeID() const { return _nodeID; };
        Rect GetRect() const { return _rect; };
        glm::vec2 GetCenter() const;
		bool GetFixed() const { return _fixed; }

		void UpdateAttribute(int attr, void* data, bool initial=false);


		DOMNode(DOMNodeType type, int64 frameID, int nodeID, Rect rect)
		{
			_type = type;
			_frameID = frameID;
			_nodeID = nodeID;
			_rect = rect;
		}

    protected:

        DOMNodeType _type;
        int64 _frameID;
        int _nodeID;			// Node's position in Javascript's list of nodes of the same type
        Rect _rect;				
		bool _fixed;


};

/* LIST OF ALL DOM NODE TYPES */

class DOMTextInput : public DOMNode
{
    public:
        DOMTextInput(	DOMNodeType type,
                        int64 frameID,
                        int nodeID,
                        Rect rect,
                        std::string value		);

        std::string GetValue() const { return _value; }

    private:

        std::string _value = "";
};



class DOMTextLink : public DOMNode
{
    public:
        DOMTextLink(	DOMNodeType type,
                        int64 frameID,
                        int nodeID,
                        Rect rect,
                        std::string text,
                        std::string url			);

        std::string GetText() const { return _text; }
        std::string GetURL() const { return _url; }

    private:
        std::string _text;
        std::string _url;
};


#endif  // DOMNODE_H_


