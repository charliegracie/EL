
add_executable(el
	Main.cpp
	CInterpreter.cpp
	CMInterpreterMethod.cpp
	IBInterpreter.cpp
	JBInterpreter.cpp
	$<TARGET_OBJECTS:bytecodes>
	$<TARGET_OBJECTS:helpers>
	$<TARGET_OBJECTS:parser>
)

target_link_libraries(el omr_jitbuilder_static)

