Test5

// should return the result of the tester function
DEF main 0
	CALL tester 0
	RET
end

DEF tester 0
	PRINT_STRING "Executing tester function\n"
	PUSH_CONSTANT 4
	RET
end
