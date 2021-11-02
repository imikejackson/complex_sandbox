cmake_minimum_required(VERSION 3.19)


project(sandbox
  VERSION 0.1.0
  DESCRIPTION "sandbox"
  HOMEPAGE_URL "https://www.github.com/imikejackson/complex_sandbox"
  LANGUAGES CXX
)


# ---------- Setup output Directories -------------------------
if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
            ${sandbox_BINARY_DIR}/Bin
            CACHE PATH
            "Single Directory for all Libraries"
            )
endif()

# --------- Setup the Executable output Directory -------------
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
            ${sandbox_BINARY_DIR}/Bin
            CACHE PATH
            "Single Directory for all Executables."
            )
endif()

# --------- Setup the Executable output Directory -------------
if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
            ${sandbox_BINARY_DIR}/Bin
            CACHE PATH
            "Single Directory for all static libraries."
            )
endif()



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
get_filename_component(sandbox_PARENT_DIR "${sandbox_SOURCE_DIR}" DIRECTORY CACHE)

#------------------------------------------------------------------------------
# If the developer has set another COMPLEX directory then use that, otherwise look
# for the COMPLEX directory at the same level as the DREAM3DNX directory
if("${COMPLEX_SOURCE_DIR}" STREQUAL "")
  if(EXISTS "${sandbox_PARENT_DIR}/complex")
    set(COMPLEX_SOURCE_DIR "${sandbox_PARENT_DIR}/complex")
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

#--------------------------------------------------------------------------------------------------
# Add in plugins
set(PROJECT_CODE_DIR ${sandbox_SOURCE_DIR})

if( "${sandbox_EXTRA_PLUGINS}" STREQUAL "")
 set(sandbox_EXTRA_PLUGINS "" CACHE STRING "")
else()
  set(sandbox_EXTRA_PLUGINS ${sandbox_EXTRA_PLUGINS} CACHE STRING "")
endif()


# --------------------------------------------------------------------
# This function optionally compiles a named plugin when compiling sandbox
# This function will add in an Option "sandbox_ENABLE_${NAME} which
# the programmer can use to enable/disable the compiling of specific plugins
# Arguments:
# PLUGIN_NAME The name of the Plugin
# PLUGIN_SOURCE_DIR The source directory for the plugin
function(sandbox_COMPILE_PLUGIN)
    set(options)
    set(oneValueArgs PLUGIN_NAME PLUGIN_SOURCE_DIR)
    cmake_parse_arguments(PLUG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    option(sandbox_ENABLE_${PLUG_PLUGIN_NAME} "Build the ${PLUG_PLUGIN_NAME}" ON)
    
    if(sandbox_ENABLE_${PLUG_PLUGIN_NAME})
        add_subdirectory(${PLUG_PLUGIN_SOURCE_DIR} ${PROJECT_BINARY_DIR}/Plugins/${PLUG_PLUGIN_NAME})

        message(STATUS "${PLUG_PLUGIN_NAME} [ENABLED] ${PluginNumFilters} Filters")
        #- Now set up the dependency between the main application and each of the plugins so that
        #- things like Visual Studio are forced to rebuild the plugins when launching
        #- the sandbox application
        if(sandbox_ENABLE_${PLUG_PLUGIN_NAME} AND TARGET sandbox AND TARGET ${PLUG_PLUGIN_NAME})
          add_dependencies(sandbox ${PLUG_PLUGIN_NAME})
        endif()

    else()
        message(STATUS "${PLUG_PLUGIN_NAME} [DISABLED]: Use -Dsandbox_ENABLE_${PLUG_PLUGIN_NAME}=ON to Enable Plugin")
    endif()
endfunction()

# --------------------------------------------------------------------
#
#
#
#
function(sandbox_ADD_PLUGINS)
    set(options)
    set(multiValueArgs PLUGIN_NAMES)
    cmake_parse_arguments(PLUG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    get_filename_component(sandbox_PARENT_DIR  ${sandbox_SOURCE_DIR} DIRECTORY)
    #message(STATUS "sandbox_PARENT_DIR: ${sandbox_PARENT_DIR}")

    #-- Attempt to look in our local source directory for the plugin. Anywhere else
    # and the user will have to put the entire path into CMake manually.
    foreach(d3dPlugin ${PLUG_PLUGIN_NAMES})

      #message(STATUS "Evaluating Plugin: ${d3dPlugin}...")

      if(DEFINED sandbox_${d3dPlugin}_SOURCE_DIR AND "${sandbox_${d3dPlugin}_SOURCE_DIR}" STREQUAL "")
        set(pluginSearchDir ${PROJECT_CODE_DIR}/Plugins/${d3dPlugin})
        set(sandbox_${d3dPlugin}_SOURCE_DIR ${pluginSearchDir} CACHE PATH "")
      endif()

      #message(STATUS "sandbox_${d3dPlugin}_SOURCE_DIR: ${sandbox_${d3dPlugin}_SOURCE_DIR}")
      if(NOT DEFINED sandbox_${d3dPlugin}_SOURCE_DIR OR NOT EXISTS "${sandbox_${d3dPlugin}_SOURCE_DIR}")
        #message(STATUS "sandbox_${d3dPlugin}_SOURCE_DIR was NOT Defined.....")
        set(pluginSearchDir ${sandbox_PARENT_DIR}/complex_plugins/${d3dPlugin})
        if(EXISTS ${pluginSearchDir})
          set(sandbox_${d3dPlugin}_SOURCE_DIR ${pluginSearchDir} CACHE PATH "")
          #message(STATUS "Plugin: Defining sandbox_${d3dPlugin}_SOURCE_DIR to ${sandbox_${d3dPlugin}_SOURCE_DIR}")
        endif()
      endif()

      # Mark these variables as advanced
      mark_as_advanced(sandbox_${d3dPlugin}_SOURCE_DIR)


    # Now that we have defined where the user's plugin directory is at we
    # need to make sure it has a CMakeLists.txt file in it
      if(EXISTS ${sandbox_${d3dPlugin}_SOURCE_DIR}/CMakeLists.txt)
        set(${d3dPlugin}_IMPORT_FILE sandbox_${d3dPlugin}_SOURCE_DIR/CMakeLists.txt)
      endif()


    # By this point we should have everything defined and ready to go...
      if(DEFINED sandbox_${d3dPlugin}_SOURCE_DIR AND DEFINED ${d3dPlugin}_IMPORT_FILE)
          #message(STATUS "Plugin: Adding Plugin ${sandbox_${d3dPlugin}_SOURCE_DIR}")
          sandbox_COMPILE_PLUGIN(PLUGIN_NAME ${d3dPlugin}
                                 PLUGIN_SOURCE_DIR ${sandbox_${d3dPlugin}_SOURCE_DIR})
      else()
          set(sandbox_${d3dPlugin}_SOURCE_DIR ${pluginSearchDir} CACHE PATH "" FORCE)
          message(FATAL_ERROR "Plugin: The directory for plugin ${d3dPlugin} was not found. Use -Dsandbox_${d3dPlugin}_SOURCE_DIR=/Path/To/PluginDir")
      endif()

    endforeach()

endfunction()

set(COMPLEX_PLUGIN_OUTPUT_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Plugins)
set_property(GLOBAL PROPERTY COMPLEX_PLUGIN_OUTPUT_DIR ${COMPLEX_PLUGIN_OUTPUT_DIR})

#-----------------
# These are the core plugins that need to be built
set(sandbox_BASE_PLUGINS
#    AFRLDistributionC
#    BrukerIntegration
#    CAxisByPolarizedLight
#    CRADA3DSystems
#    DREAM3DReview
#    ECAccess
#    EMMPM
#    Generic
#    ImageProcessing
#    ImportExport
#    ITKImageProcessing
#    NETLIntegration
#    OrientationAnalysis
#    Processing
#    ProgWorkshop
#    Reconstruction
#    Sampling
#    SimulationIO
#    StatsToolbox
#    SurfaceMeshing
#    SyntheticBuilding
#    UCSBUtilities
#    VolumeMeshing
)

#get_filename_component(sandbox_PARENT_DIR  ${sandbox_SOURCE_DIR} DIRECTORY)
set(sandbox_ALL_PLUGINS ${sandbox_BASE_PLUGINS} ${sandbox_EXTRA_PLUGINS} )

message(STATUS "*******************************************************")
message(STATUS "* Base Plugins                                        *")
message(STATUS "*******************************************************")
sandbox_ADD_PLUGINS(PLUGIN_NAMES ${sandbox_BASE_PLUGINS})


message(STATUS "*******************************************************")
message(STATUS "* Extra External Plugins                              *")
message(STATUS "*******************************************************")
sandbox_ADD_PLUGINS(PLUGIN_NAMES ${sandbox_EXTRA_PLUGINS})


