//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Global constants.

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include "src/Utils/glmWrapper.h"
#include <string>

static const int EYEGUI_WEB_URL_INPUT_LAYER = -1; // in default GUI...
static const int EYEGUI_WEB_HISTORY_LAYER = -1; // in default GUI...
static const int EYEGUI_WEB_LAYER = -2; // in default GUI...
static const int EYEGUI_TAB_LAYER = -3; // in default GUI...
static const int EYEGUI_SUPER_LAYER = 0; // in super GUI...
static const int EYEGUI_CURSOR_LAYER = 1; // in super GUI...
static const int EYEGUI_SETTINGS_LAYER = -2;
static const std::string BLANK_PAGE_URL = "about:blank";
static const int SLOTS_PER_TAB_OVERVIEW_PAGE = 5;
static const int WEB_TAB_OVERVIEW_MINI_PREVIEW_MIP_MAP_LEVEL = 3;
static const int WEB_TAB_OVERVIEW_PREVIEW_MIP_MAP_LEVEL = 0;
static const glm::vec4 TAB_DEFAULT_COLOR_ACCENT = glm::vec4(96.f / 255.f, 125.f / 255.f, 139.f / 255.f, 1.f);
static const int TAB_ACCENT_COLOR_SAMPLING_POINTS = 100;
static const float TAB_SCROLLING_SENSOR_WIDTH = 0.2f;
static const float TAB_SCROLLING_SENSOR_HEIGHT = 0.1075f;
static const float TAB_SCROLLING_DOWN_SENSOR_SCALE = 1.175f;
static const float TAB_SCROLLING_SENSOR_PADDING = 0.025f;
static const float TAB_TRIGGER_BUTTON_SIZE = 0.14f;
static const float TAB_TRIGGER_BADGE_SIZE = 0.035f;
static const glm::vec2 TAB_TRIGGER_BADGE_OFFSET = glm::vec2(0.05f, 0.05f);
static const float TAB_LOADING_ICON_FRAME_DURATION = 0.25f;
static const float TAB_GET_PAGE_RES_INTERVAL = 1.0f;
static const int TAB_DEBUGGING_GAZE_COUNT = 10;
static const float MASTER_PAUSE_ALPHA = 0.35f;
static const float EYEINPUT_MOUSE_OVERRIDE_INIT_FRAME_DURATION = 0.25f; // duration between mouse movement is expected. Triggered by initial movement
static const float EYEINPUT_MOUSE_OVERRIDE_INIT_DISTANCE = 100.f; // pixels on screen (not best but works)
static const float EYEINPUT_MOUSE_OVERRIDE_STOP_DURATION = 1.5f; // duration until override is stopped when no mouse movement done
static const std::string LOG_FILE_NAME = "log";
static const std::string INTERACTION_FILE_NAME = "interaction";
static const int LOG_FILE_MAX_SIZE = 1024 * 1024;
static const int LOG_FILE_COUNT = 5;
static const float MOUSE_CURSOR_RELATIVE_SIZE = 0.1f;
static const glm::vec3 DOM_TRIGGER_DEBUG_COLOR = glm::vec3(0, 1, 0);
static const glm::vec3 DOM_TEXT_LINKS_DEBUG_COLOR = glm::vec3(0, 0, 1);
static const glm::vec3 DOM_SELECT_FIELD_DEBUG_COLOR = glm::vec3(0, 1, 1);
static const glm::vec3 FIXED_ELEMENT_DEBUG_COLOR = glm::vec3(1, 0, 0);
static const float BLUR_FOCUS_RELATIVE_RADIUS = 0.25f; // relative to smaller of both width or height
static const float BLUR_PERIPHERY_MULTIPLIER = 0.7f;
static const std::string BOOKMARKS_FILE = "bookmarks.xml";
static const std::string HISTORY_FILE = "history.xml";
static const std::string SETTINGS_FILE = "settings.xml";
static const int URL_INPUT_BOOKMARKS_ROWS_ON_SCREEN = 6;
static const int HISTORY_ROWS_ON_SCREEN = 6;
static const int HISTORY_DISPLAY_COUNT = 20;
static const int SELECT_FIELD_OPTIONS_ON_SCREEN = 6;
static const float NOTIFICATION_WIDTH = 0.75f;
static const float NOTIFICATION_Y = 0.0075f;
static const float NOTIFICATION_HEIGHT = 0.06f;
static const float NOTIFICATION_DISPLAY_DURATION = 5.f;
static const float CLICK_VISUALIZATION_DURATION = 0.75f;
static const float CLICK_VISUALIZATION_RELATIVE_SIZE = 0.4f;
static const std::string SEARCH_PREFIX = "duckduckgo.com?q=";
static const std::string SEARCH_POSTFIX_ENGLISH = "&k1=-1&kp=-2&kl=us-en";
static const std::string SEARCH_POSTFIX_GREEK = "&k1=-1&kp=-2&kl=gr-el";
static const std::string SEARCH_POSTFIX_HEBREW = "&k1=-1&kp=-2&kl=il-he";
static const glm::vec4 NOTIFICATION_NEUTRAL_COLOR = glm::vec4(0.2f, 0.2f, 0.2f, 0.75f);
static const glm::vec4 NOTIFICATION_SUCCESS_COLOR = glm::vec4(0.15f, 1.0f, 0.0f, 0.75f);
static const glm::vec4 NOTIFICATION_WARNING_COLOR = glm::vec4(1.0f, 0.15f, 0.0f, 0.75f);
static const double FILTER_MAXIMUM_SAMPLE_AGE = std::numeric_limits<double>::max(); // maximum time returned as sample age by filter, in seconds

#endif // GLOBAL_H_
