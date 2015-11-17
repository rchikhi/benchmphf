
project(benchmphf)

cmake_minimum_required(VERSION 2.6)

################################################################################
# Define cmake modules directory
################################################################################
SET (CMAKE_MODULE_PATH  "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gatb-core/cmake")

################################################################################
# THIRD PARTIES
################################################################################

# We don't want to install some GATB-CORE artifacts
SET (GATB_CORE_EXCLUDE_TOOLS     1)
SET (GATB_CORE_EXCLUDE_TESTS     1)
SET (GATB_CORE_EXCLUDE_EXAMPLES  1)

# GATB CORE
include (GatbCore)

################################################################################
# TOOL
################################################################################

# we get compilation definitions from the gatb-core part
add_definitions (${gatb-core-flags}) 
add_definitions("-std=c++0x")  # let's force c++11 in this project


# we give the headers directories from : 
#       - from project source 
#       - from GATB-CORE source
#       - from dsk source 
include_directories (src  ${gatb-core-includes})

# we define the files to be compiled
file (GLOB_RECURSE  ProjectFiles  main.cpp)

# we define the artifact to be built: the project binary
add_executable        (${CMAKE_PROJECT_NAME}  ${ProjectFiles})

# we define which libraries to be linked with project binary
target_link_libraries (${CMAKE_PROJECT_NAME}  ${gatb-core-libraries})


