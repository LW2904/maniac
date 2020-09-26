cmake_minimum_required(VERSION 3.16)
project(maniac VERSION 1.0.0 LANGUAGES CXX)

message(STATUS "CMake version: ${CMAKE_VERSION}")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif (NOT CMAKE_BUILD_TYPE)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	message(FATAL_ERROR "Darwin is currently not supported")
endif()

add_executable(maniac
        src/main.cpp
        src/maniac/maniac.cpp
        src/process/process.cpp
        src/osu/osu.cpp
        src/maniac/humanization.cpp)

target_include_directories(maniac PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(maniac PRIVATE)

if (UNIX)
	find_package(X11 REQUIRED)
	target_link_libraries(maniac PUBLIC ${X11_LIBRARIES})

	find_path(XTEST_INCLUDE_DIR NAMES X11/extensions/XTest.h
		PATH_SUFFIXES X11/extensions
		PATHS /opt/X11/include
		DOC "The XTest include directory"
	)

	find_library(XTEST_LIBRARY NAMES Xtst
		PATHS /opt/X11/lib
		DOC "The XTest library"
	)

	include(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(XTest DEFAULT_MSG XTEST_LIBRARY XTEST_INCLUDE_DIR)

	if (XTEST_FOUND)
		set(XTEST_LIBRARIES ${XTEST_LIBRARY})
		set(XTEST_INCLUDE_DIRS ${XTEST_INCLUDE_DIR})
	endif ()

	mark_as_advanced(XTEST_INCLUDE_DIR XTEST_LIBRARY)
	target_link_libraries(maniac PUBLIC ${XTEST_LIBRARIES})
endif ()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	# The additional / is important to remove the last character from the path.
	# Note that it does not matter if the OS uses / or \, because we are only
	# saving the path size.
	# Thanks: https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
	string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
	add_compile_definitions(DEBUG SOURCE_PATH_SIZE=${SOURCE_PATH_SIZE})

	if (WIN32)
		add_compile_options(/W4 /WX /DDEBUG)
	else (UNIX)
		add_compile_options(-Wall -Wextra -Wpedantic -Werror -Og)
	endif ()
else ()
	if (WIN32)
		add_compile_options(/O2)
	else (UNIX)
		add_compile_options(-Ofast)
	endif ()
endif ()

target_compile_features(maniac PRIVATE cxx_std_17)

set_target_properties(maniac PROPERTIES OUTPUT_NAME "maniac")
