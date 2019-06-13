
# required if linking to static library
add_definitions(-DANTLR4CPP_STATIC)
set(DANTLR4_TAG 4.7.2)

# add external build for antlrcpp
include(ExternalAntlr4Cpp)

include_directories(${ANTLR4_INCLUDE_DIRS})

set(ANTLR_EXECUTABLE ${PROJECT_SOURCE_DIR}/bytecodecompiler/antlr-4.7.2-complete.jar)
find_package(ANTLR REQUIRED)

antlr_target(elparser EL.g4 LISTENER VISITOR)

include_directories(${ANTLR_elparser_OUTPUT_DIR})

add_executable(elc
	Main.cpp
	${ANTLR_elparser_CXX_OUTPUTS}
)

target_link_libraries(elc bytecodes antlr4_static)


