# GazeTheWeb - Browse

![Logo](media/Logo.png)

Gaze-controlled Web browser, part of the EU-funded research project MAMEM. There exists a deprecated prototype for initial testing purposes in [_Prototype_](Prototype) subfolder and the work in progress implementation of the full featured Web browser in the [_Client_](Client) subfolder. Both will only compile on either Windows with Visual Studio 2015 as 32bit project or on Linux with GCC 5.x as 64bit project for the moment. In addition, your graphics card must support OpenGL 3.3 or higher (f.e. not the case for second generation Intel i-GPUs or lower, at least on Windows). Prototype build has been deactivated in the current version of the CMakeLists.

## Videos
* [Demonstration](https://www.youtube.com/watch?v=x1ESgaoQR9Y)
* [Demonstration of prototype](https://www.youtube.com/watch?v=zj1u6QTmk5k)

## News
* [Announcement on official MAMEM page](http://www.mamem.eu/gazetheweb-prototype-for-gaze-controlled-browsing-the-web)
* [First user experience](http://www.mamem.eu/mamem-meets-three-remarkable-women)

## HowTo
Since the CEF3 binaries for Windows and Linux do not like each other, one has to copy them manually into the cloned project. Just follow these easy steps:

1. Clone this repository.
2. Download either Windows 32bit or Linux 64bit CEF 3.x binaries in standard distribution from [here](http://opensource.spotify.com/cefbuilds/index.html)
 * Windows Version: CEF 3.3071.1634.g9cc59c8 / Chromium 59.0.3071.82
 * Linux Version: CEF 3.3071.1636.g2718da5 / Chromium 59.0.3071.82
3. Extract the downloaded files and copy following content into the locally cloned repository:
 * include
 * libcef_dll
 * Release
 * Debug
 * Resources
 * README.txt
 * LICENSE.txt
 * **DO NOT** overwrite the provided CMakeLists.txt and the content of cmake folder.
5. Folder structure should look like this screenshot:
 * ![Folder structure](media/Folder.png)
5. If prototype should be built too, one has to include its subdirectory in the main CMakeLists, line 532.
6. Create a build folder somewhere and execute CMake to generate a project, which can be compiled.

## Notes
This project uses the _Chromium Embedded Framework_. Please visit https://bitbucket.org/chromiumembedded/cef for more information about that project!
