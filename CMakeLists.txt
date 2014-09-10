# Copyright (c) Cameron Garnham, Tigusoft, Monetas AG,  2014

cmake_minimum_required(VERSION 2.8)

project(opentxs-cli)

set(VERSION_MAJOR  0)
set(VERSION_MINOR  3)
set(VERSION_BUGFIX 0)
set(VERSION "0.3.0")


#-----------------------------------------------------------------------------
# Options for building

option(BUILD_VERBOSE       "Verbose build output." ON)
option(BUILD_TESTS         "Build the unit tests." ON)
option(KEYRING_FLATFILE    "Build with Flatfile Keyring" OFF)
option(RPM                 "Build a RPM" OFF)
option(DEB                 "Build a DEB" OFF)

if(BUILD_VERBOSE)
  set(CMAKE_VERBOSE_MAKEFILE true)
endif()


option(LOCAL_EDITLINE "Use local Editline library ($HOME/.local)" ON) # Always ON because of bugs in Debian libedit package!
option(WITH_WRAPPER "Build with wrapper" OFF)
option(LOCAL_OT "Build with locally installed ot" ON)
option(CHECK_COMPILER_VERSION "Check if current compiler version is suported" ON)
option(WITH_TERMCOLORS "Build with support for unix terminal console colors VT100" ON)


#-----------------------------------------------------------------------------
# Print system information

message(STATUS "CMake version:      ${CMAKE_VERSION}")
message(STATUS "System:             ${CMAKE_SYSTEM}")
message(STATUS "Processor:          ${CMAKE_SYSTEM_PROCESSOR}")

message(STATUS "Verbose:            ${BUILD_VERBOSE}")
message(STATUS "Testing:            ${BUILD_TESTS}")
message(STATUS "Flatfile keyring:   ${KEYRING_FLATFILE}")
message(STATUS "Build RPM:          ${RPM}")
message(STATUS "Build DEB:          ${DEB}")


#-----------------------------------------------------------------------------
# Collect all binaries into bin subdirectory

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


#-----------------------------------------------------------------------------
# Check which compiler to use

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
  INCLUDE(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG(-std=c++11 HAVE_STD11)
  if(HAVE_STD11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
  else()
    message(FATAL_ERROR "No advanced standard C++ support (-std=c++11 not defined).")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
  INCLUDE(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG(-std=c++11 HAVE_STD11)
  if(HAVE_STD11)
    if(APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
      set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -stdlib=libc++")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
    endif()
  else()
    message(FATAL_ERROR "No C++11 support for Clang version. Please upgrade Clang to a version supporting C++11.")
  endif()
endif()

if(BUILD_SHARED_LIBS)
  # Windows would complain about unknown flag -fPIC
  if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  endif()
endif()


if(MSVC)
  add_definitions(-D_UNICODE)
else()
  set(PEDANTIC_CXX_FLAGS "-Wall -Werror ${WARN_CXX_FLAGS} -Wextra -pedantic -Wno-missing-braces")
endif()

message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Flags:       ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ link flags:  ${CMAKE_CXX_LINK_FLAGS}")


#-----------------------------------------------------------------------------
# Force out-of-source build

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
message(FATAL_ERROR "CMake generation is not allowed within the source directory!
Remove the CMakeCache.txt file and try again from another folder, e.g.:

   rm CMakeCache.txt
   mkdir build
   cd build
   cmake ..
")
endif()


#-----------------------------------------------------------------------------
# Default to Release build

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif()


#-----------------------------------------------------------------------------
# Source Definitions

if(KEYRING_FLATFILE)
  add_definitions(-DOT_KEYRING_FLATFILE)
endif()

if(WIN32)
    add_definitions("-DEXPORT=__declspec(dllexport)")
else()
  add_definitions(-DEXPORT=)
endif()


# Set variables

set(CMAKE_SKIP_BUILD_RPATH  FALSE) # Always include RPATH to the binary

# This is needed to properly set RPATH for newer libstdc++ from gcc 4.8.3 (local build)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ENV{HOME}/.local/lib64:$ENV{HOME}/.local/lib")

if (WITH_WRAPPER)
	set(CMAKE_CXX_COMPILER "$ENV{HOME}/.local/bin/compile-cxx")
	set(CMAKE_CC_COMPILER "$ENV{HOME}/.local/bin/compile-cc")
	message(STATUS "Option was set to use WRAPPER. Using CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
else ()
	message(STATUS "Option was NOT set for wrapper. Using CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
endif (WITH_WRAPPER)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/") # Add folder with cmake modules

if (WITH_TERMCOLORS)
	add_definitions( -DCFG_WITH_TERMCOLORS )
endif ()

# Add executable

file(GLOB othint_SRC # All files in directory:
	"*.h"
	"*.hpp"
	"*.cpp"
)

add_executable(othint ${othint_SRC})

# Find libraries and headers for Editline

if (LOCAL_EDITLINE)
	find_package(EditlineLocal REQUIRED)
	if (EDITLINE_FOUND)
		include_directories(${EDITLINE_INCLUDE_DIR})
		target_link_libraries (othint ${EDITLINE_LIBRARIES})
	endif (EDITLINE_FOUND)
else ()
	find_package(Editline REQUIRED)
	if (EDITLINE_FOUND)
		include_directories(${EDITLINE_INCLUDE_DIR})
		target_link_libraries (othint ${EDITLINE_LIBRARIES})
	endif (EDITLINE_FOUND)
endif(LOCAL_EDITLINE)

# Find libraries and headers for OT

if(MSVC)
  include_directories(
    ${PROJECT_SOURCE_DIR}/../opentxs/include
    ${PROJECT_SOURCE_DIR}/../opentxs/include/opentxs/core
    ${PROJECT_SOURCE_DIR}/../opentxs/deps
  )
  link_directories(
    ${PROJECT_SOURCE_DIR}/../opentxs/build/lib
  )
  set(core opentxs-core)
  set(ext opentxs-ext)
  set(basket opentxs-basket)
  set(cash opentxs-cash)
else()
  find_path(opentxsIncludePath opentxs/core/OTPseudonym.hpp)

  include_directories(
    ${opentxsIncludePath}
    ${opentxsIncludePath}/opentxs/core/
  )

  find_library(core opentxs-core)
  find_library(ext opentxs-ext)
  find_library(cash opentxs-cash)
  find_library(basket opentxs-basket)
endif(MSVC)

# make run
add_custom_target(run
		COMMAND bash start-devel.sh
    DEPENDS othint
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
