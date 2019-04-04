PerfGCD

DEF main 0
	// Call gcd(24562488, 68985648) and print the result and how long it took 20 times
	PUSH_CONSTANT 20
	POP_LOCAL 2
	PUSH_CONSTANT 0
	POP_LOCAL 1
LOOP_START:
	PUSH_CONSTANT 24562488
	PUSH_CONSTANT 68985648
	CURRENT_TIME
	POP_LOCAL 0
	CALL gcd 2
	CURRENT_TIME
	PUSH_LOCAL 0
	SUB
	POP_LOCAL 0
	PRINT_STRING "gcd(24562488, 68985648) = "
	PRINT_INT64
	PRINT_STRING " executed in "
	PUSH_LOCAL 0
	PRINT_INT64
	PRINT_STRING "ms\n"

	PUSH_LOCAL 1
	PUSH_CONSTANT 1
	ADD
	DUP
	POP_LOCAL 1
	PUSH_LOCAL 2
	JMPL LOOP_START
	
	PUSH_CONSTANT 1
	RET
end

DEF gcd 2
	//N is local 0
	//i is local 1
	//a is local 2
	//b is local 3
	//hcf is local 4
	PUSH_ARG 0 //a        [a]
	DUP
	POP_LOCAL 2
	PUSH_ARG 1 //b        [a,b]
	DUP
	POP_LOCAL 3
	JMPL L1 // a < b        []
	PUSH_ARG 1 //push b        [b]
	POP_LOCAL 0 //n = b        []
	JMP L2
L1:
	PUSH_ARG 0 //push a        [a]
	POP_LOCAL 0 //n = a        []
L2:                              // []
	PUSH_CONSTANT 1
	POP_LOCAL 4            //hcf = 1
	
	PUSH_CONSTANT 2        //[2]
	POP_LOCAL 1 //i == 2        []	
	
	PUSH_LOCAL 0 //n        [n]
	PUSH_CONSTANT 3        // [n,3]
	JMPL EXIT //if n < 3        []

	
FOR_LOOP_START:                 // []
WHILE_START:                    // []
//	PUSH_LOCAL 2 //a        [a]
//	PUSH_LOCAL 1 //i        [a,2]
//	MOD          //a % i        [a%i]
	PUSH_LOCAL 3 //b        [a%i,b]
	PUSH_LOCAL 1 //i        [a%i,b,i]
	MOD          //b % i        [a%i,b%i]
	PUSH_CONSTANT 0        //[a%i,b%i,0]
	JMPE BEQUAL0 //if b % i == 0 jmp        [a%i]
//	POP //get rid of a % i         []
LOOP_INCREMENT:                       // []
	PUSH_LOCAL 1          //push i        [i]
	PUSH_CONSTANT 1       //push 1        [i,1]
	ADD                   //add i + 1        [i+1]
	DUP                   //stack is i+1 and i+1        [i+1,i+1]
	POP_LOCAL 1           //i is i + 1        [i+1]
	PUSH_LOCAL 0          //push n        [i+1,n]
	JMPL FOR_LOOP_START   //if i < n go to loop start        []
	JMP EXIT              //else goto exit                   []
BEQUAL0:                  //stack is a % i        [a%i]
	
	PUSH_LOCAL 2
	PUSH_LOCAL 1
	MOD

	PUSH_CONSTANT 0        //[a%i,0]
	JMPE AEQUAL0       // []
	JMP LOOP_INCREMENT       //[]
AEQUAL0:                       //  []
	PUSH_LOCAL 4   //push hcf        [hcf]
	PUSH_LOCAL 1   //push i        [hcf,i]
	MUL                             // [hcf*i]
	POP_LOCAL 4    //hcf = hcf * i        []
	PUSH_LOCAL 2       // [a]
	PUSH_LOCAL 1       // [a,i]
	DIV                  //     [a/i]
	POP_LOCAL 2    //a = a / i         []
	PUSH_LOCAL 3      //  [b]
	PUSH_LOCAL 1       // [b,i]
	DIV                 //      [b/i]
	POP_LOCAL 3    //b = b / i           []
	JMP WHILE_START        //           []
EXIT:                      //       []
	PUSH_LOCAL 4      //  [hcf]
	RET
end