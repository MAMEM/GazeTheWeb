//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "Master.h"
#include "src/Utils/Helper.h"
#include "src/Utils/Logger.h"
#include "submodules/glfw/include/GLFW/glfw3.h"
#include "submodules/text-csv/include/text/csv/ostream.hpp"
#include <functional>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>
#include <fstream>

// Namespace for text-csv
namespace csv = ::text::csv;

// Shaders for screen filling rendering
const std::string vertexShaderSource =
"#version 330 core\n"
"void main() {\n"
"}\n";

const std::string geometryShaderSource =
"#version 330 core\n"
"layout(points) in;\n"
"layout(triangle_strip, max_vertices = 4) out;\n"
"out vec2 uv;\n"
"void main() {\n"
"    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);\n"
"    uv = vec2(1.0, 1.0);\n"
"    EmitVertex();\n"
"    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
"    uv = vec2(0.0, 1.0);\n"
"    EmitVertex();\n"
"    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);\n"
"    uv = vec2(1.0, 0.0);\n"
"    EmitVertex();\n"
"    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
"    uv = vec2(0.0, 0.0);\n"
"    EmitVertex();\n"
"    EndPrimitive();\n"
"}\n";

const std::string blurFragmentShaderSource =
"#version 330 core\n"
"const float offset = 1.9;\n" // TODO: maybe use resolution here
"in vec2 uv;\n"
"out vec4 fragColor;\n"
"uniform sampler2D tex;\n"
"uniform vec2 focusPixelPosition;\n"
"uniform float focusPixelRadius;\n"
"uniform float peripheryMultiplier;\n"
"void main() {\n"
// Preparation
"   vec4 color = texture(tex, uv);\n"
"   float mask = min(distance(focusPixelPosition, gl_FragCoord.xy) / focusPixelRadius, 1.0);\n"
"   vec2 texSize = textureSize(tex, 0);\n"
"   vec4 blur = vec4(0,0,0,0);\n"
// x and y not zero
"   for(int x = 1; x <= 2; x++) {\n"
"       for(int y = 1; y <= 2; y++) {\n"
"           blur += texture(tex, (gl_FragCoord.xy + (offset * vec2(x, y))) / texSize);\n"
"           blur += texture(tex, (gl_FragCoord.xy + (offset * vec2(-x, y))) / texSize);\n"
"           blur += texture(tex, (gl_FragCoord.xy + (offset * vec2(x, -y))) / texSize);\n"
"           blur += texture(tex, (gl_FragCoord.xy + (offset * vec2(-x, -y))) / texSize);\n"
"       }\n"
"   }\n"
// x is zero
"   for(int y = 1; y <= 2; y++) {\n"
"       blur += texture(tex, (gl_FragCoord.xy + (offset * vec2(0, y))) / texSize);\n"
"       blur += texture(tex, (gl_FragCoord.xy + (offset * vec2(0, -y))) / texSize);\n"
"   }\n"
// y is zero
"   for(int x = 1; x <= 2; x++) {\n"
"       blur += texture(tex, (gl_FragCoord.xy + (offset * vec2(x, 0))) / texSize);\n"
"       blur += texture(tex, (gl_FragCoord.xy + (offset * vec2(-x, 0))) / texSize);\n"
"   }\n"
// Both is zero
"   blur += color;\n"
// Do composition
"   blur /= 25;\n"
"   color = mix(color, peripheryMultiplier * blur, mask);\n"
"   fragColor = vec4(color.rgb, 1.0);\n"
"}\n";

const std::string simpleFragmentShaderSource =
"#version 330 core\n"
"in vec2 uv;\n"
"out vec4 fragColor;\n"
"uniform sampler2D tex;\n"
"void main() {\n"
"   fragColor = texture(tex, uv);\n"
"}\n";

Master::Master(Mediator* pCefMediator, std::string userDirectory)
{
    // Save members
    _pCefMediator = pCefMediator;
	_userDirectory = userDirectory;

    // ### GLFW AND OPENGL ###

    // Create OpenGL context
    LogInfo("Initializing GLFW...");
    glfwInit();
    LogInfo("..done.");

    // Window mode and size
    GLFWmonitor* usedMonitor = NULL;
    if (setup::FULLSCREEN)
    {
        LogInfo("Fullscreen mode");
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode * mode = glfwGetVideoMode(monitor);
        _width = mode->width;
        _height = mode->height;
        usedMonitor = monitor;
    }
    else
    {
        LogInfo("Windowed mode");
    }

    LogInfo("Creating window...");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    _pWindow = glfwCreateWindow(_width, _height, "GazeTheWeb - Browse", usedMonitor, NULL);
    glfwMakeContextCurrent(_pWindow);
    glfwSetInputMode(_pWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN); // hide native mouse cursor
    LogInfo("..done.");
    LogInfo("Initializing OpenGL...");
    ogl_LoadFunctions();
    LogInfo("..done.");

    // Log hardware and maximum OpenGL version
    const GLubyte* renderer = glGetString(GL_RENDERER);
    LogInfo("GPU: ", std::string(reinterpret_cast<char const*>(renderer)));
    const GLubyte* version = glGetString(GL_VERSION);
    LogInfo("OpenGL Version: ", std::string(reinterpret_cast<char const*>(version)));

    // VSync
    glfwSwapInterval(1);
#ifdef _WIN32
    // Turn on vertical screen sync under Windows
    // (I.e. it uses the WGL_EXT_swap_control extension)
    typedef BOOL(WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int interval);
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT)
        wglSwapIntervalEXT(1);
#endif

    // Register callbacks to GLFW
    static std::function<void(int, int, int, int)> kC = [&](int k, int s, int a, int m) { this->GLFWKeyCallback(k, s, a, m); };
    glfwSetKeyCallback(_pWindow, [](GLFWwindow* window, int k, int s, int a, int m) { kC(k, s, a, m); });

    static std::function<void(int, int, int)> bC = [&](int b, int a, int m) { this->GLFWMouseButtonCallback(b, a, m); };
    glfwSetMouseButtonCallback(_pWindow, [](GLFWwindow* window, int b, int a, int m) { bC(b, a, m); });

    static std::function<void(double, double)> cC = [&](double x, double y) { this->GLFWCursorPosCallback(x, y); };
    glfwSetCursorPosCallback(_pWindow, [](GLFWwindow* window, double x, double y) { cC(x, y); });

    static std::function<void(int, int)> fC = [&](int w, int h) { this->GLFWResizeCallback(w, h); };
    glfwSetFramebufferSizeCallback(_pWindow, [](GLFWwindow* window, int w, int h) { fC(w, h); });

	// ### WINDOW ICON ###

	// Set content path (before using it in the helper)
	eyegui::setRootFilepath(CONTENT_PATH);

	// Load window icons for GLFW
	std::vector<unsigned char> icon16Data;
	int icon16Width, icon16Height, icon16ChannelCount;
	eyegui_helper::loadImage("resources/Icon_16.png", icon16Data, icon16Width, icon16Height, icon16ChannelCount);

	std::vector<unsigned char> icon32Data;
	int icon32Width, icon32Height, icon32ChannelCount;
	eyegui_helper::loadImage("resources/Icon_32.png", icon32Data, icon32Width, icon32Height, icon32ChannelCount);

	std::vector<unsigned char> icon64Data;
	int icon64Width, icon64Height, icon64ChannelCount;
	eyegui_helper::loadImage("resources/Icon_64.png", icon64Data, icon64Width, icon64Height, icon64ChannelCount);

	// Create GLFWImages
	std::vector<GLFWimage> icons;
	icons.resize(3);
	icons.at(0).width = icon16Width;
	icons.at(0).height = icon16Height;
	icons.at(0).pixels = icon16Data.data();
	icons.at(1).width = icon32Width;
	icons.at(1).height = icon32Height;
	icons.at(1).pixels = icon32Data.data();
	icons.at(2).width = icon64Width;
	icons.at(2).height = icon64Height;
	icons.at(2).pixels = icon64Data.data();

	// Set window icon
	glfwSetWindowIcon(
		_pWindow,
		(int)icons.size(),
		icons.data());

    // ### EYEGUI ###

    // Set print callbacks
    std::function<void(std::string)> printGUICallback = [&](std::string message) { this->GUIPrintCallback(message); };
    eyegui::setErrorCallback(printGUICallback);
    eyegui::setWarningCallback(printGUICallback);

	// Create GUI builder
	LogInfo("Creating eyeGUI...");
	eyegui::GUIBuilder guiBuilder;
	guiBuilder.width = _width;
	guiBuilder.height = _height;
	guiBuilder.fontFilepath = "fonts/dejavu-sans/ttf/DejaVuSans.ttf";
	guiBuilder.characterSet = eyegui::CharacterSet::US_ENGLISH;
	guiBuilder.localizationFilepath = "localizations/English.leyegui";
	guiBuilder.fontTallSize = 0.07f;

	// Create splash screen GUI, render it one time and throw it away
	eyegui::GUI* pSplashGUI = guiBuilder.construct();
	eyegui::addLayout(pSplashGUI, "layouts/Splash.xeyegui"); // TODO: fill version string
	eyegui::updateGUI(pSplashGUI, 1.f, eyegui::Input()); // update GUI one time for resizing
	eyegui::drawGUI(pSplashGUI);
	glfwSwapBuffers(_pWindow);
	glfwPollEvents(); // poll events not necessary for GUI but lets display the icon in Windows taskbar earlier
	eyegui::terminateGUI(pSplashGUI);

    // Construct GUI
    _pGUI = guiBuilder.construct(); // standard GUI object used everywhere
    _pSuperGUI = guiBuilder.construct(); // GUI which is rendered on top of everything else
    LogInfo("..done.");

    // Load configuration
    eyegui::loadConfig(_pGUI, "configuration/Global.ceyegui");
    eyegui::loadConfig(_pSuperGUI, "configuration/Global.ceyegui");

    // Set resize callback of GUI
    std::function<void(int, int)> resizeGUICallback = [&](int width, int height) { this->GUIResizeCallback(width, height); };
    eyegui::setResizeCallback(_pGUI, resizeGUICallback); // only one GUI needs to callback it. Use standard for it

    // Load dictionary
    _dictonaryId = eyegui::addDictionary(_pGUI, "dictionaries/NewEnglishUS.dic");

    // ### INTERACTION LOGGING ###

    // New CSV file for interaction logging
    if(setup::LOG_INTERACTIONS)
    {
        // Name of file
        std::string filename = std::string(INTERACTION_FILE_NAME) + ".csv";

        // Title line
        LogInfo("Create file for interaction logging...");
        LogInfo("File named: ", filename);
        std::ofstream fs(filename, std::ios_base::out); // overwrite existing
        csv::csv_ostream csvs(fs);
        csvs << "Timestamp" << "Layout" << "GazeCoordinate" << "ElementType" << "ElementId" << "ElementRect" << "ElementActivity" << "InteractionType" << "InteractionInfoA";
		fs << std::endl;
        LogInfo("..done.");

        // Listen to eyeGUI
        eyegui::setInteractionCallback([filename](
            std::string layout,
            std::string gazeCoordinate,
            std::string elementType,
            std::string elementId,
            std::string elementRect,
            std::string elementActivity,
            std::string interactionType,
            std::string interactionInfoA)
        {
            // Get current date and time
            const auto currentTime = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(currentTime);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X");

            // Get current miliseconds
            auto currentTimeRounded = std::chrono::system_clock::from_time_t(time);
            if(currentTimeRounded > currentTime)
            {
                --time;
                currentTimeRounded -= std::chrono::seconds(1);
            }
            int milliseconds = std::chrono::duration_cast<std::chrono::duration<int,std::milli> >(currentTime - currentTimeRounded).count();

            // Write everything to file
            std::ofstream fs(filename, std::ios_base::app | std::ios_base::out); // append to existing
            csv::csv_ostream csvs(fs);
			csvs
				<< ss.str() + ":" + std::to_string(milliseconds)
				<< layout
				<< gazeCoordinate
				<< elementType
				<< elementId
				<< elementRect
				<< elementActivity
				<< interactionType
				<< interactionInfoA;
			fs << std::endl;
        });
    }

    // ### STATES ###

    // Create states
    _upWeb = std::unique_ptr<Web>(new Web(this, _pCefMediator));
    _upSettings = std::unique_ptr<Settings>(new Settings(this));

    // Set first state
    _currentState = StateType::WEB;
    _upWeb->Activate();

    // ### HOMEPAGE ###
	_upWeb->AddTab(_upSettings->GetHomepage());

    // ### SUPER LAYOUT ###

    // Load layout (deleted at eyeGUI termination)
    _pSuperLayout = eyegui::addLayout(_pSuperGUI, "layouts/Super.xeyegui", EYEGUI_SUPER_LAYER, true);

    // Button listener for pause
    _spMasterButtonListener = std::shared_ptr<MasterButtonListener>(new MasterButtonListener(this));
    eyegui::registerButtonListener(_pSuperLayout, "pause", _spMasterButtonListener);

    // Initialization
    if(setup::PAUSED_AT_STARTUP)
    {
        eyegui::buttonDown(_pSuperLayout, "pause", true);
    }
    _pausedDimming.setValue(0);

	// Add floating frame for notification
	_notificationFrameIndex = eyegui::addFloatingFrameWithBrick(
		_pSuperLayout,
		"bricks/Notification.beyegui",
		(1.f - NOTIFICATION_WIDTH) / 2.f,
		NOTIFICATION_Y,
		NOTIFICATION_WIDTH,
		NOTIFICATION_HEIGHT,
		false,
		false);
	
	// ### CURSOR LAYOUT ###

    // Add floating frame on empty layout for cursor
    _pCursorLayout = eyegui::addLayout(_pSuperGUI, "layouts/Empty.xeyegui", EYEGUI_CURSOR_LAYER, true);
    eyegui::setInputUsageOfLayout(_pCursorLayout, false);
    _cursorFrameIndex = eyegui::addFloatingFrameWithBrick(_pCursorLayout, "bricks/Cursor.beyegui", 0, 0, 0, 0, true, false); // will be moved and sized in loop

    // ### INPUT ###
    _upEyeInput = std::unique_ptr<EyeInput>(new EyeInput);

    // ### FRAMEBUFFER ###
    _upFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(_width, _height));
	_upFramebuffer->Bind();
    _upFramebuffer->AddAttachment(Framebuffer::ColorFormat::RGB);
	_upFramebuffer->Unbind();
    _upScreenFillingQuad = std::unique_ptr<RenderItem>(
        new RenderItem(
            vertexShaderSource,
            geometryShaderSource,
            setup::BLUR_PERIPHERY ? blurFragmentShaderSource : simpleFragmentShaderSource));

	// ### JavaScript to LSL ###

	// Registers a JavaScript callback function that pipes JS callbacks starting with "lsl:" to LabStreamingLayer
	_pCefMediator->RegisterJavascriptCallback("lsl:", [this](std::string message) { LabStreamMailer::instance().Send(message); });

	// ### LabStreamCallback ###

	// Create callback
	_spLabStreamCallback = std::shared_ptr<LabStreamCallback>(new LabStreamCallback(
		[](std::vector<std::string> messages)
		{
			for (const std::string& rMessage : messages)
			{
				LogInfo("LabStream: " + rMessage);
			}
		}
	));

	// Register callback
	LabStreamMailer::instance().RegisterCallback(_spLabStreamCallback);

    // ### OTHER ###

    // Time
    _lastTime = glfwGetTime();
}

Master::~Master()
{
    // Manual destruction of Web. Otherwise there are errors in CEF at shutdown (TODO: understand why)
    _upWeb.reset();

    // Terminate eyeGUI
    eyegui::terminateGUI(_pSuperGUI);
    eyegui::terminateGUI(_pGUI);

    // Terminate GLFW
    glfwTerminate();
}

void Master::Run()
{
    this->Loop();
}

double Master::GetTime() const
{
    return glfwGetTime();
}

void Master::Exit()
{
	// Close all tabs
	_upWeb->RemoveAllTabs();

	// Stop update loop
	_exit = true;

	// Let CEF do a last message loop to be able to clean up
	_pCefMediator->DoMessageLoopWork();
}

void Master::PushNotification(std::u16string content)
{
	_notificationStack.push(content);
}

void Master::PushNotificationByKey(std::string key)
{
	_notificationStack.push(eyegui::fetchLocalization(_pGUI, key));
}

eyegui::Layout* Master::AddLayout(std::string filepath, int layer, bool visible)
{
    // Add layout
    eyegui::Layout* pLayout = eyegui::addLayout(_pGUI, filepath, layer, visible);
    return pLayout;
}

void Master::RemoveLayout(eyegui::Layout* pLayout)
{
    // Remove layout
    eyegui::removeLayout(_pGUI, pLayout);
}

std::u16string Master::FetchLocalization(std::string key) const
{
	return eyegui::fetchLocalization(_pGUI, key);
}

void Master::Loop()
{
	while (!_exit)
	{
		// Call exit when window should close
		if (glfwWindowShouldClose(_pWindow))
		{
			Exit();
			continue; // or break because _exit is set by Exit()
		}

		// Time per frame
		double currentTime = glfwGetTime();
		float tpf = std::min((float)(currentTime - _lastTime), 0.25f); // everything breaks when tpf too big
		_lastTime = currentTime;

		// Decrement time until input is accepted
		if (_timeUntilInput > 0)
		{
			_timeUntilInput -= tpf;
		}

		// Update lab streaming layer mailer to get incoming messages
		LabStreamMailer::instance().Update();

		// Poll CefMediator
		_pCefMediator->Poll(tpf);

		// Notification handling
		if (_notificationTime <= 0)
		{
			// Show next notification
			if (!_notificationStack.empty())
			{
				// Set content
				auto content = _notificationStack.front();
				eyegui::setContentOfTextBlock(
					_pSuperLayout,
					"notification",
					content);
				_notificationStack.pop();

				// Make floating frame visible
				eyegui::setVisibilityOFloatingFrame(_pSuperLayout, _notificationFrameIndex, true, false, true);

				// Reset time
				_notificationTime = NOTIFICATION_DISPLAY_DURATION;
			}
			else
			{
				// Hide notification display
				eyegui::setVisibilityOFloatingFrame(_pSuperLayout, _notificationFrameIndex, false, false, true);
			}
		}
		else
		{
			_notificationTime -= tpf;
			_notificationTime = glm::max(0.f, _notificationTime);
		}

        // Get cursor coordinates
        double currentMouseX;
        double currentMouseY;
        glfwGetCursorPos(_pWindow, &currentMouseX, &currentMouseY);

        // Update eye input
		int windowX = 0;
		int windowY = 0;
		glfwGetWindowPos(_pWindow, &windowX, &windowY);
        double gazeX;
        double gazeY;
        bool gazeUsed = _upEyeInput->Update(
			tpf,
			currentMouseX,
			currentMouseY,
			gazeX,
			gazeY,
			windowX,
			windowY,
			_width,
			_height);

        // Update cursor
        eyegui::setVisibilityOfLayout(_pCursorLayout, !gazeUsed, false, true);
        float halfRelativeMouseCursorSize = MOUSE_CURSOR_RELATIVE_SIZE / 2.f;
        eyegui::setPositionOfFloatingFrame(
            _pCursorLayout,
            _cursorFrameIndex,
            (gazeX / _width) - halfRelativeMouseCursorSize,
            (gazeY / _height) - halfRelativeMouseCursorSize);
        eyegui::setSizeOfFloatingFrame(
            _pCursorLayout,
            _cursorFrameIndex,
            MOUSE_CURSOR_RELATIVE_SIZE,
            MOUSE_CURSOR_RELATIVE_SIZE);

        // Pause visualization
        _pausedDimming.update(tpf, !_paused);
        eyegui::setValueOfStyleAttribute(
            _pSuperLayout,
            "pause_background",
            "background-color",
            RGBAToHexString(glm::vec4(0, 0, 0, MASTER_PAUSE_ALPHA * _pausedDimming.getValue())));

        // Input struct for eyeGUI
        eyegui::Input eyeGUIInput;

        // Fill input structure
        eyeGUIInput.instantInteraction = (_leftMouseButtonPressed && !gazeUsed) || (_enterKeyPressed && gazeUsed);
        eyeGUIInput.gazeX = (int)gazeX;
        eyeGUIInput.gazeY = (int)gazeY;

        // Check for focus and time until input
        int focused = glfwGetWindowAttrib(_pWindow, GLFW_FOCUSED);
        if((focused <= 0) // window not focused
            || (_timeUntilInput > 0)) // do not use input, yet
        {
            // TODO: Do it more effeciently (like calling it with NULL instead of reference)
            eyeGUIInput.gazeUsed = true;
        }

        // Update eyeGUI
        eyegui::Input usedEyeGUIInput = eyegui::updateGUI(_pSuperGUI, tpf, eyeGUIInput);

        if(_paused)
        {
            // Do not pipe input to standard GUI if paused
            usedEyeGUIInput.gazeUsed = true; // TODO: null pointer would be nicer
        }
        usedEyeGUIInput = eyegui::updateGUI(_pGUI, tpf, usedEyeGUIInput);

        // Do message loop of CEF
        _pCefMediator->DoMessageLoopWork();

        // Create input struct for own framework
        Input input(usedEyeGUIInput.gazeX, usedEyeGUIInput.gazeY, usedEyeGUIInput.gazeUsed, usedEyeGUIInput.instantInteraction);

        // Bind framebuffer
        _upFramebuffer->Bind();

        // Clearing of buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Disable depth test for drawing
		glDisable(GL_DEPTH_TEST);

        // Update current state (one should use here pointer instead of switch case)
        StateType nextState = StateType::WEB;
        switch (_currentState)
        {
        case StateType::WEB:
            nextState = _upWeb->Update(tpf, input);
            _upWeb->Draw();
            break;
        case StateType::SETTINGS:
            nextState = _upSettings->Update(tpf, input);
            _upSettings->Draw();
            break;
        }

        // Check next state
        if (_currentState != nextState)
        {
            // Deactivate current state
            switch (_currentState)
            {
            case StateType::WEB:
                _upWeb->Deactivate();
                break;
            case StateType::SETTINGS:
                _upSettings->Deactivate();
                break;
            }

            // Activate next state
            switch (nextState)
            {
            case StateType::WEB:
                _upWeb->Activate();
                break;
            case StateType::SETTINGS:
                _upSettings->Activate();
                break;
            }

            // Remember state
            _currentState = nextState;
        }

		// Enable depth test again
		glEnable(GL_DEPTH_TEST);

        // Draw eyeGUI on top
        eyegui::drawGUI(_pGUI);
        eyegui::drawGUI(_pSuperGUI);

        // Bind standard framebuffer
        _upFramebuffer->Unbind();

        // Clearing of buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind framebuffer as texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _upFramebuffer->GetAttachment(0));

        // Render screen filling quad
        _upScreenFillingQuad->Bind();

        // Fill uniforms when necessary
        if(setup::BLUR_PERIPHERY)
        {
            _upScreenFillingQuad->GetShader()->UpdateValue("focusPixelPosition", glm::vec2(usedEyeGUIInput.gazeX, _height - usedEyeGUIInput.gazeY)); // OpenGL coordinate system
            _upScreenFillingQuad->GetShader()->UpdateValue("focusPixelRadius", (float)glm::min(_width, _height) * BLUR_FOCUS_RELATIVE_RADIUS);
            _upScreenFillingQuad->GetShader()->UpdateValue("peripheryMultiplier", BLUR_PERIPHERY_MULTIPLIER);
        }

         _upScreenFillingQuad->Draw(GL_POINTS);

        // Reset reminder BEFORE POLLING
        _leftMouseButtonPressed = false;
        _enterKeyPressed = false;

        // Swap front and back buffers and poll events
        glfwSwapBuffers(_pWindow);
        glfwPollEvents();
    }
}

void Master::GLFWKeyCallback(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_ESCAPE: { Exit(); break; }
            case GLFW_KEY_TAB:  { eyegui::hitButton(_pSuperLayout, "pause"); break; }
            case GLFW_KEY_ENTER: { _enterKeyPressed = true; break; }
			case GLFW_KEY_S: { LabStreamMailer::instance().Send("42"); break; } // TODO: testing
			case GLFW_KEY_0: { _pCefMediator->ShowDevTools(); break; }
        }
    }
}

void Master::GLFWMouseButtonCallback(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        _leftMouseButtonPressed = true;
    }
}

void Master::GLFWCursorPosCallback(double xpos, double ypos)
{
    // Nothing to do, yet
}

void Master::GLFWResizeCallback(int width, int height)
{
    // Save it
    _width = width;
    _height = height;

    // Tell it eyeGUI (which indirect tells its Tabs which tell it CEF...)
    eyegui::resizeGUI(_pGUI, _width, _height);
    eyegui::resizeGUI(_pSuperGUI, _width, _height);

    // Set viewport (can be set here, untouched/rescued by eyeGUI)
    // Independent from bound framebuffer
    glViewport(0, 0, _width, _height);

    // Tell framebuffer about new window size
    _upFramebuffer->Bind();
    _upFramebuffer->Resize(_width, _height);
    _upFramebuffer->Unbind();

    // CEF mediator is told to resize tabs via GUI callback
}

void Master::GUIResizeCallback(int width, int height)
{
    // Tell CEF mediator to resize tabs
    // Has to be done after GUI has resized because size of web view is taken from GUI
    _pCefMediator->ResizeTabs();
}

void Master::GUIPrintCallback(std::string message) const
{
    LogInfo("eyeGUI: ", message);
}

void Master::MasterButtonListener::down(eyegui::Layout* pLayout, std::string id)
{
    if(pLayout == _pMaster->_pSuperLayout)
    {
        _pMaster->_paused = true;
        eyegui::setDescriptionVisibility(_pMaster->_pGUI, eyegui::DescriptionVisibility::VISIBLE);
    }
}

void Master::MasterButtonListener::up(eyegui::Layout* pLayout, std::string id)
{
    if(pLayout == _pMaster->_pSuperLayout)
    {
        _pMaster->_paused = false;
        eyegui::setDescriptionVisibility(_pMaster->_pGUI, eyegui::DescriptionVisibility::ON_PENETRATION); // TODO look up in Settings for set value
    }
}
