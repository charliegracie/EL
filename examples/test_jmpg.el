TestJMPG

DEF main 0
	PUSH_CONSTANT 10
	PUSH_CONSTANT 9
	JMPG L1
	PUSH_CONSTANT 73
	RET
L1:
	PUSH_CONSTANT 4
	PUSH_CONSTANT 5
	JMPG L2
	PUSH_CONSTANT 27
	RET
L2:
	PUSH_CONSTANT 31
	RET
end
