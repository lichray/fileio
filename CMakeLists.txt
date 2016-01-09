cmake_minimum_required(VERSION 2.8.11)
project(fileio CXX)
enable_testing()

include(CTest)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY tests)
set(CMAKE_CXX_STANDARD 14)

if(NOT MSYS)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(NOT MSVC)
	add_definitions(-Wall)
else()
	foreach(flagvar CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
	    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		string(REGEX REPLACE "/MD" "/MT" ${flagvar} "${${flagvar}}")
		string(REGEX REPLACE "/MDd" "/MTd" ${flagvar} "${${flagvar}}")
	endforeach()
endif()

include_directories(include)

file(GLOB fileio_srcs src/*.cc src/*.cpp)
file(GLOB tests_srcs tests/*.cc tests/*.t.cpp)

add_library(fileio STATIC ${fileio_srcs})

if(BUILD_TESTING)
	foreach(test_src ${tests_srcs})
		get_filename_component(test_suite ${test_src} NAME_WE)
		add_executable(${test_suite} ${test_src})
		target_link_libraries(${test_suite} fileio)
		add_test(${test_suite} tests/${test_suite})
	endforeach()
endif()
