
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

message(STATUS "Starting with CMake version ${CMAKE_VERSION}")

project(EL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EL_VERSION_MAJOR 0)
set(EL_VERSION_MINOR 1)

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

include(ELDetectCompilerInformation)

el_detect_compiler_information()

include(toolcfg/${EL_TOOLCONFIG})

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
	set(EL_IS_BIG_ENDIAN true CACHE BOOL "")
else()
	set(EL_IS_LITTLE_ENDIAN true CACHE BOOL "")
endif()

configure_file (
	"${PROJECT_SOURCE_DIR}/runtime/ELConfig.h.in"
	"${PROJECT_BINARY_DIR}/runtime/ELConfig.h"
)

include(ExternalOMR)
include_directories(${OMR_INCLUDE_DIRS})

include_directories(
	"${PROJECT_BINARY_DIR}/runtime"
	"${PROJECT_SOURCE_DIR}/runtime"
	"${PROJECT_SOURCE_DIR}/bytecodes"
	"${PROJECT_SOURCE_DIR}/parser"
	"${PROJECT_SOURCE_DIR}/helpers"
)

add_subdirectory(helpers)
add_subdirectory(parser)
add_subdirectory(bytecodes)
add_subdirectory(runtime)
add_subdirectory(demo)
add_subdirectory(bytecodecompiler)
