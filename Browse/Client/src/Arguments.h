//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Program arguments parsed at CefInitialization by MainCefApp and before
// construction of Master.

class Argument
{
public:

	enum class Localization
	{
		English, Greek, Hebrew
	};
	static Localization localization;
};