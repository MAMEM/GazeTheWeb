cmake_minimum_required(VERSION 2.8)

# Only generate Debug and Release configuration types
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Setup project
project(GazeTheWeb-BrowseUpdater)
set(APPNAME BrowseUpdater)

# Set release build as default
set(CMAKE_BUILD_TYPE Release)

# Activate C++11 in GCC and CLANG
if(NOT MSVC)
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)
endif(NOT MSVC)

### SOURCES ####################################################################

# Add include path to directory
include_directories(${CMAKE_SOURCE_DIR})

# Collect own code
file(GLOB_RECURSE SOURCES
	"${CMAKE_SOURCE_DIR}/src/*.cpp"
	"${CMAKE_SOURCE_DIR}/src/*.c"
)
file(GLOB_RECURSE HEADERS
	"${CMAKE_SOURCE_DIR}/src/*.h"
)

### EXTERNALS ##################################################################

# Set path to externals
set(EXTERNALS_DIR "${CMAKE_SOURCE_DIR}/externals")

# Curl
add_definitions(-DCURL_STATICLIB)
set(CURL_DIR "${EXTERNALS_DIR}/curl")
set(CURL_LIBRARIES "${CURL_DIR}/lib/static-release-x86/libcurl_a.lib")
	
### SUBMODULES #################################################################

# Set path to submodules
set(SUBMODULES_DIR "${CMAKE_SOURCE_DIR}/submodules")

# miniz
set(MINIZ_DIR "${SUBMODULES_DIR}/miniz")
set(MINIZ_SOURCES
"${MINIZ_DIR}/miniz.c"
"${MINIZ_DIR}/miniz_zip.c"
"${MINIZ_DIR}/miniz_tinfl.c"
"${MINIZ_DIR}/miniz_tdef.c")

# Collect all code
set(ALL_CODE
	${SOURCES}
	${HEADERS}
	${MINIZ_SOURCES})

### FILTERING ##################################################################

# Filtering for Visual Studio
if(MSVC)

	# http://stackoverflow.com/questions/9701387/cmake-source-group-multiple-files
	foreach(f ${ALL_CODE})
		# Get the path of the file relative to ${CMAKE_CURRENT_SOURCE_DIR},
		# then alter it (not compulsory)
		file(RELATIVE_PATH SRCGR "${CMAKE_CURRENT_SOURCE_DIR}" ${f})
		set(SRCGR "${APPNAME}/${SRCGR}")

		# Extract the folder, ie remove the filename part
		string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})

		# Source_group expects \\ (double antislash), not / (slash)
		string(REPLACE / \\ SRCGR ${SRCGR})
		source_group("${SRCGR}" FILES ${f})
	endforeach()

endif(MSVC)

### CREATION AND LINKING #######################################################

# Creation of executeable
add_executable(${APPNAME} ${ALL_CODE})

# Linking
target_link_libraries(${APPNAME} ${CURL_LIBRARIES})
