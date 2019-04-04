TestPrintString

DEF main 0
	PRINT_STRING "this is my test\nthat includes new lines\nI hope!\n"
	PRINT_STRING "another string\n"
	CALL func 0
	RET
end

DEF func 0
	PRINT_STRING "first string\n"
	PRINT_STRING "another string\n"
	PUSH_CONSTANT 23
	RET
end

