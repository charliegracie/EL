add_executable(demo
	DemoInterpreter.cpp
	DemoMethod.cpp
	DemoHelpers.cpp
	Main.cpp
	$<TARGET_OBJECTS:bytecodes>
	$<TARGET_OBJECTS:parser>
)

target_link_libraries(demo omr_jitbuilder_static)
