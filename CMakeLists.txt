cmake_minimum_required(VERSION 3.19)


project(sandbox
  VERSION 0.1.0
  DESCRIPTION "sandbox"
  HOMEPAGE_URL "https://www.github.com/imikejackson/complex_sandbox"
  LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)



set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#--------------------------------------------------------------------------------------------------
# Always write out the compile_commands.json file to help out things like QtCreator and VS Code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#--------------------------------------------------------------------------------------------------
# Find and include the `complex` repository. This will invoke vcpkg to ensure the dependent libraries
# are all downloaded and available
#--------------------------------------------------------------------------------------------------
get_filename_component(sandbox_PARENT "${sandbox_SOURCE_DIR}" DIRECTORY CACHE)
#------------------------------------------------------------------------------
# If the developer has set another COMPLEX directory then use that, otherwise look
# for the COMPLEX directory at the same level as the DREAM3DNX directory
if("${COMPLEX_SOURCE_DIR}" STREQUAL "")
  if(EXISTS "${sandbox_PARENT}/complex")
    set(COMPLEX_SOURCE_DIR "${sandbox_PARENT}/complex")
  else()
    message(FATAL_ERROR "COMPLEX_SOURCE_DIR was not set. Where is the COMPLEX project directory. Please set the COMPLEX_SOURCE_DIR variable to the COMPLEX directory.")
  endif()
  message(STATUS "COMPLEX_SOURCE_DIR: ${COMPLEX_SOURCE_DIR}")
endif()
#--------------------------------------------------------------------------------------------------
# Build the `complex` library first
add_subdirectory("${COMPLEX_SOURCE_DIR}" ${sandbox_BINARY_DIR}/complex)


#--------------------------------------------------------------------------------------------------
# Add in our own targets and executables
include ("${sandbox_SOURCE_DIR}/sandbox/SourceList.cmake")
