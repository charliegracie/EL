
add_library(helpers
	OBJECT
		BytecodeHelpers.cpp
		Helpers.cpp
)

target_link_libraries(helpers omr_jitbuilder_static)

include_directories(
	"/Users/charliegracie/git/omr/include_core"
	"/Users/charliegracie/git/omr/jitbuilder/release/cpp/include"
)
