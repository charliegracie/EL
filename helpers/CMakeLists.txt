
add_library(helpers
	BytecodeHelpers.cpp
	Helpers.cpp
)

target_link_libraries(helpers omr_jitbuilder_static)

