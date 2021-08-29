### CLIENT #####################################################################

set(CEF_TARGET Client)

message(STATUS "*** CLIENT ***")

set(CLIENT_MAJOR_VERSION "1")
set(CLIENT_MINOR_VERSION "9")
add_definitions(-DCLIENT_MAJOR_VERSION=${CLIENT_MAJOR_VERSION})
add_definitions(-DCLIENT_MINOR_VERSION=${CLIENT_MINOR_VERSION})
add_definitions(-DCLIENT_VERSION="${CLIENT_MAJOR_VERSION}.${CLIENT_MINOR_VERSION}")

string(TIMESTAMP DATE %d/%m/%Y)

### PARAMETERS #################################################################

if(OS_WINDOWS) # Windows

	# OpenGaze
	# TODO: GazePoint client uses MFC which results in a linker error, as MFC has to be linked before CRT but no idea how to
	# set(CLIENT_BUILD_OPEN_GAZE_PLUGIN ON CACHE BOOL "Build plugin for OpenGaze API.")

	# SMI iViewX
	set(CLIENT_BUILD_SMI_IVIEWX_PLUGIN ON CACHE BOOL "Build plugin for SMI iViewX.")
	set(CLIENT_SMI_IVIEWX_SDK_PATH "C:/Program Files (x86)/SMI/iView X SDK" CACHE PATH "Path to SMI iViewX SDK.")
	
	# Visual Interaction myGaze
	set(CLIENT_BUILD_VI_MYGAZE_PLUGIN ON CACHE BOOL "Build plugin for Visual Interaction myGaze.")
	set(CLIENT_VI_MYGAZE_SDK_PATH "C:/Program Files (x86)/Visual Interaction/myGaze SDK" CACHE PATH "Path to Visual Interaction myGaze SDK.")
	
	# Tobii EyeX
	set(CLIENT_BUILD_TOBII_EYEX_PLUGIN ON CACHE BOOL "Build plugin for Tobii EyeX.")
	set(CLIENT_TOBII_EYEX_SDK_PATH "C:/TobiiEyeXSDK-CPP/TobiiEyeXSDK" CACHE PATH "Path to Tobii EyeX SDK.")
	
	# Tobii Pro
	set(CLIENT_BUILD_TOBII_PRO_PLUGIN OFF CACHE BOOL "Build plugin for Tobii Pro.")
	set(CLIENT_TOBII_PRO_SDK_PATH "C:/tobii_research" CACHE PATH "Path to Tobii Pro SDK.")
	
	# Sensor Lib integration
	set(CLIENT_SENSOR_LIB_INTEGRATION OFF CACHE BOOL "Integrate Sensor Lib.")
	
endif()

# Deployment prepares ready to share binary version
set(CLIENT_DEPLOYMENT OFF CACHE BOOL "Setup for Deployment.")
if(${CLIENT_DEPLOYMENT})
	add_definitions(-DCLIENT_DEPLOYMENT)
endif()

# Create Installer prepares Script to create an installer.
set(CLIENT_CREATE_INSTALLER OFF CACHE BOOL "Setup for installer creation.")

# Debug mode
set(CLIENT_DEBUG OFF CACHE BOOL "Setup for Debugging.")
if(${CLIENT_DEBUG})
	add_definitions(-DCLIENT_DEBUG)
endif()

# Demo mode
set(CLIENT_DEMO OFF CACHE BOOL "Setup for Demonstration.")
if(${CLIENT_DEMO})
	add_definitions(-DCLIENT_DEMO)
endif()

# Sensor Lib integration
if(${CLIENT_SENSOR_LIB_INTEGRATION})
	add_definitions(-DCLIENT_SENSOR_LIB_INTEGRATION)
endif()

if(OS_LINUX) # Linux
	add_definitions(-Doff_t=__off_t)
endif()

if(OS_WINDOWS) # Windows
	add_definitions(/MT) # Do not use MSVC RUNTIME_LIBRARY_DLL but static one. And no MTd, as CEF only uses MT.
	add_definitions(-D_HAS_EXCEPTIONS=0) # CEF settings disable exceptions. Tell it VS, so no warnings are generated.
endif()

### CONTENT ####################################################################

# Only used when NOT in deployment mode
set(CLIENT_CONTENT_PATH ${CMAKE_CURRENT_LIST_DIR}/content CACHE PATH "Path to content folder.")
add_definitions(-DCONTENT_PATH="${CLIENT_CONTENT_PATH}")
add_subdirectory("${CLIENT_CONTENT_PATH}")

### SOURCES ####################################################################

# Paths
set(CLIENT_SRC_PATH "${CMAKE_CURRENT_LIST_DIR}/src")
set(CLIENT_COMMON_PATH "${CMAKE_CURRENT_LIST_DIR}/common")

# Include directory
include_directories("${CMAKE_CURRENT_LIST_DIR}")

# Collect own code
file(GLOB_RECURSE SOURCES
	"${CLIENT_SRC_PATH}/*.js"
	"${CLIENT_SRC_PATH}/*.cpp"
	"${CLIENT_SRC_PATH}/*.c"
	"${CLIENT_COMMON_PATH}/*.cpp"
	"${CLIENT_COMMON_PATH}/*.c")

file(GLOB_RECURSE HEADERS
	"${CLIENT_SRC_PATH}/*.h"
	"${CLIENT_COMMON_PATH}/*.h")
	
### EXTERNALS ##################################################################

# Set path to externals
set(EXTERNALS_DIR "${CMAKE_CURRENT_LIST_DIR}/externals")

# Add OpenGLLoader
file(GLOB OGL
	"${EXTERNALS_DIR}/OGL/*.c")
include_directories(externals/OGL)

# LabStreamingLayer 
set(LIBLSL_DIRECTORY "${EXTERNALS_DIR}/liblsl")
include_directories("${LIBLSL_DIRECTORY}/include")
if(OS_WINDOWS) # Windows

	# LabStreamingLayer (dynamic linking)
	set(LIBLSL_LIBRARIES "${LIBLSL_DIRECTORY}/lib-vs2015_x86_release/liblsl32.lib")

elseif(OS_LINUX) # Linux

	# LabStreamingLayer (static linking)
	set(LIBLSL_LIBRARIES_DEBUG "${LIBLSL_DIRECTORY}/lib-gcc_x64_debug/liblsl.a")
	set(LIBLSL_LIBRARIES_RELEASE "${LIBLSL_DIRECTORY}/lib-gcc_x64_release/liblsl.a")
	set(LIBLSL_BOOST_LIBRARIES_DEBUG "${LIBLSL_DIRECTORY}/lib-gcc_x64_debug/libboost.a")
	set(LIBLSL_BOOST_LIBRARIES_RELEASE "${LIBLSL_DIRECTORY}/lib-gcc_x64_release/libboost.a")

	# Add liblsl to debug and release builds
	ADD_LOGICAL_TARGET("lsl_lib" "${LIBLSL_LIBRARIES_DEBUG}" "${LIBLSL_LIBRARIES_RELEASE}")
	ADD_LOGICAL_TARGET("lsl_boost_lib" "${LIBLSL_BOOST_LIBRARIES_DEBUG}" "${LIBLSL_BOOST_LIBRARIES_RELEASE}")

endif()

# CURL
if(WIN32) # Windows
	set(CURL_DIRECTORY "${EXTERNALS_DIR}/curl")
	set(CURL_INCLUDE_DIR "${CURL_DIRECTORY}/include")
	set(CURL_LIBRARIES_RELEASE "${CURL_DIRECTORY}/lib/static-release-x86/libcurl_a.lib")
	set(CURL_LIBRARIES_DEBUG "${CURL_DIRECTORY}/lib/static-debug-x86/libcurl_a_debug.lib")
	ADD_LOGICAL_TARGET("curl_lib" "${CURL_LIBRARIES_DEBUG}" "${CURL_LIBRARIES_RELEASE}")
	add_definitions(-DCURL_STATICLIB) # tell MSVC to use static lib of curl
else() # Linux
	# ${CURL_LIBRARIES}
	# ${CURL_INCLUDE_DIR}
	find_package(CURL REQUIRED)
endif()
include_directories(${CURL_INCLUDE_DIR})

# go-speech-recognition-lib
if(WIN32) # Windows
	set(GO_SPEECH_DIRECTORY "${EXTERNALS_DIR}/go-speech-recognition-lib")
	set(GO_SPEECH_INCLUDE_DIR "${GO_SPEECH_DIRECTORY}")
else() # Linux
	# TODO future
endif()
include_directories(${GO_SPEECH_INCLUDE_DIR})


### SUBMODULES #################################################################

# Set path to submodules
set(SUBMODULES_DIR "${CMAKE_CURRENT_LIST_DIR}/submodules")

# eyeGUI
set(EYEGUI_DIRECTORY "${SUBMODULES_DIR}/eyeGUI")
include_directories("${EYEGUI_DIRECTORY}/include")
add_subdirectory(${EYEGUI_DIRECTORY})

# GLFW3
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "GLFW build examples." )
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "GLFW build tests.")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "GLFW build docs.")
set(GLFW_INSTALL OFF CACHE INTERNAL "GLFW install.")
set(GLFW_DOCUMENT_INTERNALS OFF CACHE INTERNAL "GLFW document internals.")
set(GLFW_USE_EGL OFF CACHE INTERNAL "GLFW use EGL.")
set(GLFW_USE_HYBRID_HPG OFF CACHE INTERNAL "GLFW use hybrid HPG.")
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE INTERNAL "MSCV runtime library dll.")
set(GLFW_VULKAN_STATIC OFF CACHE INTERNAL "Use the Vulkan loader statically linked into application.")
set(LIB_SUFFIX "" CACHE INTERNAL "Suffix of lib.")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "GLFW build shared libs.")
add_subdirectory(${SUBMODULES_DIR}/glfw)

# GLM
include_directories("${SUBMODULES_DIR}/GLM")

# spdlog
include_directories("${SUBMODULES_DIR}/spdlog/include")

# spdlog
include_directories("${SUBMODULES_DIR}/spdlog/include")

# TinyXML2 already included in eyeGUI

# OpenGL (found this way on all systems)
# ${OPENGL_LIBRARIES}
# ${OPENGL_INCLUDE_DIR}
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

### CREATION AND LINKING #######################################################

# Collect all code
set(ALL_CODE
	${SOURCES}
	${HEADERS}
	${OGL})

# Logical target used to link the libcef library
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

# Determine the target output directory
SET_CEF_TARGET_OUT_DIR()

if(OS_WINDOWS) # Windows

	# Sensor Lib integration
	if(${CLIENT_SENSOR_LIB_INTEGRATION})
	
		set(SENSOR_LIB_DIRECTORY "${EXTERNALS_DIR}/sensorLib")
	
		# Include path
		include_directories("${SENSOR_LIB_DIRECTORY}/include")
		
		# Library to link
		set(SENSOR_LIB_LIBRARIES "${SENSOR_LIB_DIRECTORY}/lib/SensorLibrary.lib")
		
	endif()

	# TODO: find reason for that console output instead of just not displaying it
	set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "/NODEFAULTLIB:LIBCMT" )

	# Resource file
	set(RESOURCE_FILE "${CMAKE_CURRENT_LIST_DIR}/Resources.rc")

	# Creation of executable
	add_executable(${CEF_TARGET} WIN32 ${ALL_CODE} ${RESOURCE_FILE})
	SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})

	# TODO: put resources in extra source group

	# Add dependencies
	add_dependencies(${CEF_TARGET} libcef_dll_wrapper)

	# Linking of libraries
	target_link_libraries(
		${CEF_TARGET}
		${OPENGL_LIBRARIES}
		glfw
		eyeGUI
		${LIBLSL_LIBRARIES}
		${SENSOR_LIB_LIBRARIES}
		curl_lib
		libcef_lib
		libcef_dll_wrapper
		${CEF_STANDARD_LIBS})

	if(USE_SANDBOX)
		# Logical target used to link the cef_sandbox library
		ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
		SET_TARGET_PROPERTIES(${CEF_TARGET} PROPERTIES LINK_FLAGS -LTCG) # CEF compilation output tells one to do so
		target_link_libraries(${CEF_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS} )
	endif()

	# Add the custom manifest files to the executable
	ADD_WINDOWS_MANIFEST("${CMAKE_CURRENT_SOURCE_DIR}" "${CEF_TARGET}" "exe")

	# Copy binary and resource files to the target output directory
	COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
	COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

	# Copy LabStreamingLayer library to output folder
	add_custom_command(TARGET ${CEF_TARGET} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${LIBLSL_DIRECTORY}/lib-vs2015_x86_release/liblsl32.dll"
		$<TARGET_FILE_DIR:${CEF_TARGET}>)

	# Copy Go speech library to output folder
	add_custom_command(TARGET ${CEF_TARGET} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${GO_SPEECH_DIRECTORY}/go-speech-recognition.dll"
		$<TARGET_FILE_DIR:${CEF_TARGET}>)
		
	# Copy Sensor Lib library and dependencies to output folder (relies on liblsl.dll provided by GTW)
	if(${CLIENT_SENSOR_LIB_INTEGRATION})
	
	# Copy all dependencies....
	macro(SimpleCopy arg)
		add_custom_command(TARGET ${CEF_TARGET} PRE_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${SENSOR_LIB_DIRECTORY}/bin/${arg}"
			$<TARGET_FILE_DIR:${CEF_TARGET}>)
	endmacro()
	
	# Libraries
	SimpleCopy("SensorLibrary.dll")
	SimpleCopy("EBNeuro.Galileo.Hardware.Communication.Device.GSR.dll")
	SimpleCopy("EBNeuro.Galileo.Hardware.IO.dll")
	SimpleCopy("EBNeuro.Galileo.Hardware.Ut.dll")
	SimpleCopy("MathNet.Numerics.dll")
	SimpleCopy("SDK_Shimmer.dll")
	SimpleCopy("ShimmerClosedLibraryRev0_4.dll")
	
	# Licenses
	SimpleCopy("LICENSE-EBNeuro.x.txt")
	SimpleCopy("LICENSE-MathNet.Numerics.txt")
	SimpleCopy("LICENSE-SDK_Shimmer.txt")
	SimpleCopy("LICENSE-SensorLibrary.txt")
	SimpleCopy("LICENSE-ShimmerClosedLibraryRev0_4.txt")
	
	endif()
	
elseif(OS_LINUX) # Linux

	# Add custom compiler flags (necessary for spdlog)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions" )

	# Creation of executeable
	add_executable(${CEF_TARGET} ${ALL_CODE})
	SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})

	# Add dependencies
	add_dependencies(${CEF_TARGET} libcef_dll_wrapper)

	# Linking of libraries
	target_link_libraries(
		${CEF_TARGET}
		${OPENGL_LIBRARIES}
		glfw
		eyeGUI
		lsl_lib
		lsl_boost_lib
		${CURL_LIBRARIES}
		libcef_lib
		libcef_dll_wrapper
		${CEF_STANDARD_LIBS})

	# Set rpath so that libraries can be placed next to the executable
	set_target_properties(${CEF_TARGET} PROPERTIES INSTALL_RPATH "$ORIGIN")
	set_target_properties(${CEF_TARGET} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
	set_target_properties(${CEF_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR})

	# Copy binary and resource files to the target output directory
	COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
	COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

	# Set SUID permissions on the chrome-sandbox target
	SET_LINUX_SUID_PERMISSIONS("${CEF_TARGET}" "${CEF_TARGET_OUT_DIR}/chrome-sandbox")

endif()

### FILTERING ##################################################################

# Filtering for Visual Studio
if(MSVC)

	# http://stackoverflow.com/questions/9701387/cmake-source-group-multiple-files
	foreach(f ${ALL_CODE})
		# Get the path of the file relative to ${CMAKE_CURRENT_SOURCE_DIR},
		# then alter it (not compulsory)
		file(RELATIVE_PATH SRCGR "${CMAKE_CURRENT_SOURCE_DIR}" ${f})
		set(SRCGR "${SRCGR}")

		# Extract the folder, ie remove the filename part
		string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})

		# Source_group expects \\ (double antislash), not / (slash)
		string(REPLACE / \\ SRCGR ${SRCGR})
		source_group("${SRCGR}" FILES ${f})
	endforeach()

endif(MSVC)

### DEPLOYMENT #################################################################

if(CLIENT_DEPLOYMENT)

	# Create text file with version information
	configure_file(VERSION.in ${CMAKE_BINARY_DIR}/VERSION) # first, create it in build folder
	add_custom_command(TARGET ${CEF_TARGET} PRE_BUILD # then, copy it to specific binary folder (direct creation there does not work)
		COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_BINARY_DIR}/VERSION
		$<TARGET_FILE_DIR:${CEF_TARGET}>/VERSION)
		
	# Generate end-user readme
	configure_file(README.in ${CMAKE_BINARY_DIR}/README.txt) # first, create it in build folder
	add_custom_command(TARGET ${CEF_TARGET} PRE_BUILD # then, copy it to specific binary folder (direct creation there does not work)
		COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_BINARY_DIR}/README.txt
		$<TARGET_FILE_DIR:${CEF_TARGET}>/README.txt)

	# Copy List of Voice Commands
	configure_file(VoiceCommands.in ${CMAKE_BINARY_DIR}/VoiceCommands.txt)
	add_custom_command(TARGET ${CEF_TARGET} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_BINARY_DIR}/VoiceCommands.txt
		$<TARGET_FILE_DIR:${CEF_TARGET}>/VoiceCommands.txt)

	# Copy content to output folder
	add_custom_command(TARGET ${CEF_TARGET} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CLIENT_CONTENT_PATH}
		$<TARGET_FILE_DIR:${CEF_TARGET}>/content)
		
	# Copy Windows tile styling
	add_custom_command(
	TARGET ${CEF_TARGET} PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_LIST_DIR}/Client.VisualElementsManifest.xml
			$<TARGET_FILE_DIR:${CEF_TARGET}>/Client.VisualElementsManifest.xml)
		
endif(CLIENT_DEPLOYMENT)

if(CLIENT_CREATE_INSTALLER)

	if (NOT CLIENT_DEPLOYMENT)
		message(WARNING "CLIENT_DEPLOYMENT wasn't set, it's needed to be set to properly create the installer.") # i.e. the installer script depends on the existence of the "VERSION" file
	endif (NOT CLIENT_DEPLOYMENT)
	configure_file(InstallerCreator.in ${CMAKE_BINARY_DIR}/InstallerCreator.nsi @ONLY)
	
	configure_file(LICENSE ${CMAKE_BINARY_DIR}/LICENSE COPYONLY)
	add_custom_command(TARGET ${CEF_TARGET} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_BINARY_DIR}/LICENSE
		$<TARGET_FILE_DIR:${CEF_TARGET}>/LICENSE)


endif(CLIENT_CREATE_INSTALLER)

 
### EYETRACKER PLUGINS #########################################################

set(EYETRACKER_PLUGIN_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/plugins/Eyetracker")

# Plugin for OpenGaze API (broken, as MFC do not work here. use other implementation for networking)
if(${CLIENT_BUILD_OPEN_GAZE_PLUGIN})

	# Plugin project
	add_library(
		OpenGazePlugin
		MODULE
		${EYETRACKER_PLUGIN_DIRECTORY}/Interface/Eyetracker.h
		${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerSample.h
		${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerInfo.h
		${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerGeometry.h
		${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.h
		${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.cpp
		${EYETRACKER_PLUGIN_DIRECTORY}/OpenGaze/OpenGazeImpl.cpp
		${EYETRACKER_PLUGIN_DIRECTORY}/OpenGaze/include/GPClient.cpp
		${CLIENT_COMMON_PATH}/LabStream/LabStream.cpp)
		
	# Link LSL
	target_link_libraries(OpenGazePlugin ${LIBLSL_LIBRARIES})

	# Tell user about it
	message(STATUS "Plugin for OpenGaze API will be built.")
	
endif()

# Plugin for SMI iViewX
if(${CLIENT_BUILD_SMI_IVIEWX_PLUGIN})

	# Check for SDK path
	if(EXISTS ${CLIENT_SMI_IVIEWX_SDK_PATH})
	
		# SMI iViewX SDK
		include_directories("${CLIENT_SMI_IVIEWX_SDK_PATH}/include")
		set(SMI_IVIEWX_SDK_LIBRARIES "${CLIENT_SMI_IVIEWX_SDK_PATH}/lib/iViewXAPI.lib")

		# Plugin project
		add_library(
			SMIiViewXPlugin
			MODULE
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/Eyetracker.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerSample.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerInfo.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerGeometry.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.cpp
			${EYETRACKER_PLUGIN_DIRECTORY}/SMIiViewX/SMIiViewXImpl.cpp
			${CLIENT_COMMON_PATH}/LabStream/LabStream.cpp)
		
		# Link SDK and LSL
		target_link_libraries(SMIiViewXPlugin ${SMI_IVIEWX_SDK_LIBRARIES} ${LIBLSL_LIBRARIES})

		# Tell user about it
		message(STATUS "Plugin for SMI iViewX will be built.")
	
	else()
		message(WARNING "SMI iViewX SDK directory not found, plugin will *not* be built.")
	endif()
endif()

# Plugin for Visual Interaction myGaze
if(${CLIENT_BUILD_VI_MYGAZE_PLUGIN})

	# Check for SDK path
	if(EXISTS ${CLIENT_VI_MYGAZE_SDK_PATH})
	
		# Visual Interaction myGaze SDK
		include_directories("${CLIENT_VI_MYGAZE_SDK_PATH}/include")
		set(VI_MYGAZE_SDK_LIBRARIES "${CLIENT_VI_MYGAZE_SDK_PATH}/lib/myGazeAPI.lib")

		# Plugin project
		add_library(
			VImyGazePlugin
			MODULE
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/Eyetracker.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerSample.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerInfo.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerGeometry.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.cpp
			${EYETRACKER_PLUGIN_DIRECTORY}/VImyGaze/VImyGazeImpl.cpp
			${CLIENT_COMMON_PATH}/LabStream/LabStream.cpp)
		
		# Link SDK and LSL
		target_link_libraries(VImyGazePlugin ${VI_MYGAZE_SDK_LIBRARIES} ${LIBLSL_LIBRARIES})

		# Tell user about it
		message(STATUS "Plugin for Visual Interaction myGaze will be built.")
	else()
		message(WARNING "Visual Interaction myGaze SDK directory not found, plugin will *not* be built.")
	endif()
endif()

# Plugin for Tobii EyeX
if(${CLIENT_BUILD_TOBII_EYEX_PLUGIN})

	# Check for SDK path
	if(EXISTS ${CLIENT_TOBII_EYEX_SDK_PATH})
	
		# Tobii EyeX SDK
		include_directories("${CLIENT_TOBII_EYEX_SDK_PATH}/include")
		set(TOBII_EYEX_SDK_LIBRARIES "${CLIENT_TOBII_EYEX_SDK_PATH}/lib/x86/Tobii.EyeX.Client.lib")

		# Plugin project
		add_library(
			TobiiEyeXPlugin
			MODULE
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/Eyetracker.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerSample.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerInfo.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerGeometry.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.cpp
			${EYETRACKER_PLUGIN_DIRECTORY}/TobiiEyeX/TobiiEyeXImpl.cpp
			${CLIENT_COMMON_PATH}/LabStream/LabStream.cpp)
			
		# Link SDK and LSL
		target_link_libraries(TobiiEyeXPlugin ${TOBII_EYEX_SDK_LIBRARIES} ${LIBLSL_LIBRARIES})
		
		# Tell user about it
		message(STATUS "Plugin for Tobii EyeX will be built.")
		
		# Copy Tobii EyeX library to output folder
		add_custom_command(TARGET TobiiEyeXPlugin POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CLIENT_TOBII_EYEX_SDK_PATH}/lib/x86/Tobii.EyeX.Client.dll"
			$<TARGET_FILE_DIR:TobiiEyeXPlugin>)
	
	else()
		message(WARNING "Tobii EyeX SDK directory not found, plugin will *not* be built.")
	endif()
endif()

# Plugin for Tobii Pro
if(${CLIENT_BUILD_TOBII_PRO_PLUGIN})

	# Check for SDK path
	if(EXISTS ${CLIENT_TOBII_PRO_SDK_PATH})
	
		# Tobii Pro SDK
		include_directories("${CLIENT_TOBII_PRO_SDK_PATH}/32/include")
		set(TOBII_PRO_SDK_LIBRARIES "${CLIENT_TOBII_PRO_SDK_PATH}/32/lib/tobii_research.lib")

		# Plugin project
		add_library(
			TobiiProPlugin
			MODULE
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/Eyetracker.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerSample.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerInfo.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Interface/EyetrackerGeometry.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.h
			${EYETRACKER_PLUGIN_DIRECTORY}/Common/EyetrackerData.cpp
			${EYETRACKER_PLUGIN_DIRECTORY}/TobiiPro/TobiiProImpl.cpp
			${CLIENT_COMMON_PATH}/LabStream/LabStream.cpp)
			
		# Link SDK and LSL
		target_link_libraries(TobiiProPlugin ${TOBII_PRO_SDK_LIBRARIES} ${LIBLSL_LIBRARIES})
		
		# Tell user about it
		message(STATUS "Plugin for Tobii Pro will be built.")
		
		# Copy Tobii Pro library to output folder
		add_custom_command(TARGET TobiiProPlugin POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CLIENT_TOBII_PRO_SDK_PATH}/32/lib/tobii_research.dll"
			$<TARGET_FILE_DIR:TobiiProPlugin>)
	
	else()
		message(WARNING "Tobii Pro SDK directory not found, plugin will *not* be built.")
	endif()
endif()