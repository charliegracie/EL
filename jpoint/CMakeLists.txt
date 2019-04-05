add_executable(jpoint
	JPointInterpreter.cpp
	JPointMethod.cpp
	JPointHelpers.cpp
	Main.cpp
	$<TARGET_OBJECTS:bytecodes>
	$<TARGET_OBJECTS:parser>
)

target_link_libraries(jpoint omr_jitbuilder_static)
