# GazeTheWeb - Browse: Client
Gaze-controlled Web browser, part of the EU-funded research project MAMEM.

## Structure
![Structure](media/Structure.png)

## HowTo
Please refer to the Readme in the [parent folder](https://github.com/MAMEM/GazeTheWeb/tree/master/Browse) for details about compiling.

## Shortcuts
* ESC: Exit application
* Tab: Toggle pause

## Commandline Arguments
* "--voice-input": Enables voice input mode

## Validation
A file named _log.txt_ is created in user folder (_~/AppData/Roaming/GazeTheWeb/Browse_ for Windows and _~/.config/GazeTheWeb/Browse_ for Linux), containing information about the current and last runs. If anything wents not as expected, one should take a look into it.

## Issues
* eyeGUI / nanoSVG rendering of SVGs is not working correctly unter Ubuntu 16.04 in combination with Chromium Embedded Framework

## Screenshots
![Duckduckgo](media/Screenshot-A.png)

![Text Input](media/Screenshot-B.png)

![Wikipedia](media/Screenshot-C.png)

![Tab Overview](media/Screenshot-D.png)

## Dependencies
Dependencies are provided in the __externals__ and __submodules__ folder.

__externals__:
* liblsl: https://github.com/sccn/labstreaminglayer
  * Boost: https://github.com/boostorg/boost
* curl: https://github.com/curl/curl
* SensorLib: https://github.com/MAMEM/SensorLib
* go-speech-recognition-lib: https://github.com/Drizzy3D/go-speech-recognition-lib
	
__submodules__:
* eyeGUI: https://github.com/raphaelmenges/eyeGUI
  * FreeType 2.6.1: http://www.freetype.org (FreeType license chosen)
* TinyXML2 (used one from eyeGUI library): https://github.com/leethomason/tinyxml2
* GLFW3: http://www.glfw.org
* GLM: http://glm.g-truc.net/0.9.7/index.html (MIT license chosen)
* spdlog: https://github.com/gabime/spdlog
* text-csv: https://github.com/roman-kashitsyn/text-csv
* JSON: https://github.com/nlohmann/json

__plugins__:

In order to build the eye tracker plugins, one must provide the pathes to the locally installed SDKs. There are following plugins available in the __plugins__ folder:

plugins/Eyetracker/OpenGaze:
* Connection to the OpenGaze API designed by GazePoint (https://www.gazept.com). No SDK necessary but a Visual Studio installation including MFC.

plugins/Eyetracker/SMIiViewX:
* Connection to the iViewX SDK, copyright SMI GmbH (http://www.smivision.com)

plugins/Eyetracker/VImyGaze:
* Connection to the myGaze SDK, copyright Visual Interaction GmbH (http://www.mygaze.com)

plugins/Eyetracker/TobiiEyeX:
* Connection to Tobii EyeX SDK, copyright Tobii Technology AB (http://developer.tobii.com/eyex-sdk)

## License
>Copyright 2020 Raphael Menges, Daniel Müller and Christopher Dreide

>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

>		http://www.apache.org/licenses/LICENSE-2.0

>Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
