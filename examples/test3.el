Test3

// should return 9 since 6 is greater than 5
DEF main 0
	PUSH_CONSTANT 6
	PUSH_CONSTANT 5
	JMPL LESSTHAN
	JMP GREATERTHAN
LESSTHAN:
	PUSH_CONSTANT 8
	RET
GREATERTHAN:
	PUSH_CONSTANT 9
	RET
end
