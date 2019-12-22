# Copyright (c) 2017-2019 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required( VERSION 3.10 )
set(PROJECT_NAME intel_qsv_decoder)
project( ${PROJECT_NAME} )

# set CMAKE_INSTALL_PREFIX
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/build CACHE PATH "..." FORCE)
endif()
set(EXECUTABLE_OUTPUT_PATH bin)
set(LIBRARY_OUTPUT_PATH lib)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
# Disable ZERO_CHECK project
set(CMAKE_SUPPRESS_REGENERATION true)

# Default to `release` build if nothing was specified
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to release")
  set(CMAKE_BUILD_TYPE release)
endif()

if( NOT DEFINED CMAKE_MFX_HOME )
  set( CMAKE_MFX_HOME "${CMAKE_HOME_DIRECTORY}/api" )
endif()

set(TARGET_INCLUDES "")
set(TARGET_LIBRARIES "")

# IPP
find_package(IPP)
if(IPP_FOUND)
  add_definitions ( -DHAVE_IPP )
  list(APPEND TARGET_INCLUDES "${IPP_INCLUDE_DIRS}")
  list(APPEND TARGET_LIBRARIES "${IPP_LIBRARIES}")
endif()

if(WIN32)
  set(WIN_LIBRARIES ${CMAKE_HOME_DIRECTORY}/lib/libmfx_vs2015.lib)
  add_definitions("-DWIN64")
else()
  set(LIN_LIBRARIES "")
endif()

add_definitions("-DNOMINMAX")
add_definitions("-D__SSE2__")

add_library(${PROJECT_NAME} SHARED
  ${CMAKE_HOME_DIRECTORY}/src/intel_qsv_decoder_api.cpp
  ${CMAKE_HOME_DIRECTORY}/src/intel_qsv_decoder_api.h
  ${CMAKE_HOME_DIRECTORY}/src/intel_video_decoder.cpp
  ${CMAKE_HOME_DIRECTORY}/src/intel_video_decoder.h
  ${CMAKE_HOME_DIRECTORY}/src/YuvRgbConvert.cpp
  ${CMAKE_HOME_DIRECTORY}/src/YuvRgbConvert.h
)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_MFX_HOME}/include
  ${TARGET_INCLUDES}
)
target_link_libraries(${PROJECT_NAME}
  ${WIN_LIBRARIES}
  ${LIN_LIBRARIES}
  ${TARGET_LIBRARIES}
)
