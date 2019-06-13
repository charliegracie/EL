
add_executable(el
	Main.cpp
	CInterpreter.cpp
	CMInterpreterMethod.cpp
	IBInterpreter.cpp
	JBInterpreter.cpp
)

target_link_libraries(el bytecodes helpers parser omr_jitbuilder_static)

