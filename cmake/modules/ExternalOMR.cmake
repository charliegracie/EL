cmake_minimum_required(VERSION 3.7)

include(ExternalProject)

set(OMR_ROOT ${CMAKE_CURRENT_BINARY_DIR}/external_omr)
set(OMR_INCLUDE_DIRS ${OMR_ROOT}/include_core ${OMR_ROOT}/jitbuilder/release/cpp/include)
set(OMR_GIT_REPOSITORY https://github.com/charliegracie/omr.git)
set(OMR_TAG conf2019)
set(OMR_JITBUILDER_STATIC_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/omr_jitbuilder/src/omr_jitbuilder-build/jitbuilder/libjitbuilder.a)

set(OMR_BUILD_COMMAND
	${CMAKE_COMMAND}
	--build .
	--target jitbuilder)
	
set(jit_builder_args
	-DOMR_COMPILER:BOOL=ON
	-DOMR_JITBUILDER:BOOL=ON
)
	

ExternalProject_Add(
	omr_jitbuilder
	PREFIX omr_jitbuilder
	GIT_REPOSITORY ${OMR_GIT_REPOSITORY}
	GIT_TAG ${OMR_TAG}
	DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
	BUILD_COMMAND ${OMR_BUILD_COMMAND}
	BUILD_IN_SOURCE 0
	SOURCE_DIR ${OMR_ROOT}
	SOURCE_SUBDIR ""
	CMAKE_CACHE_ARGS ${jit_builder_args}
	INSTALL_COMMAND ""
	EXCLUDE_FROM_ALL 1
	BUILD_BYPRODUCTS ${OMR_JITBUILDER_STATIC_LIBRARY})
	
add_library(omr_jitbuilder_static STATIC IMPORTED)
set_target_properties(omr_jitbuilder_static PROPERTIES IMPORTED_LOCATION ${OMR_JITBUILDER_STATIC_LIBRARY})
add_dependencies(omr_jitbuilder_static omr_jitbuilder)

