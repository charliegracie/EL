/**
 * Define a grammar called eL
 */
grammar EL;

program
	: programName definition+ EOF
	;
	
programName
	: NAME
	;
	
definition
	: functionDeclaration
	;
	
functionDeclaration
	: 'DEF' functionName functionArgCount functionBody eos
	;
	
functionName
	: NAME
	;
	
functionArgCount
	: integer
	;
	
functionBody
	: instruction+
	;
	
instruction
	: noArgInstruction
	| oneArgInstruction
	| callInstruction
	| jumpInstruction
	| printStringInstruction
	| label
	;
	
noArgInstruction
	: instructionName
	;
	
oneArgInstruction
	: instructionName integer 
	;
	
callInstruction
	: instructionName functionName integer 
	;
	
jumpInstruction
	: instructionName labelName 
	;
	
printStringInstruction
	: instructionName string 
	;
	
label
	: labelName ':' 
	;
	
labelName
	: NAME
	;
	
string
	: STRING
	;

eos
	: 'end' 
	;
	
instructionName
	: 'NOP'
	| 'PUSH_CONSTANT'
	| 'PUSH_ARG'
	| 'PUSH_LOCAL'
	| 'POP'
	| 'POP_LOCAL'
	| 'DUP'
	| 'ADD'
	| 'SUB'
	| 'MUL'
	| 'DIV'
	| 'MOD'
	| 'JMP'
	| 'JMPE'
	| 'JMPL'
	| 'JMPG'
	| 'CALL'
	| 'RET'
	| 'PRINT_STRING'
	| 'PRINT_INT64'
	| 'CURRENT_TIME'
	| 'HALT'
	;
	
integer
	: INT
	;
	
INT
	: [0-9]+
	;

NAME
	: [a-zA-Z][a-zA-Z0-9_]*
	;

STRING
	: '"' ~('"')* '"'
	;
	
WHITESPACE
	:[ \t]+ -> skip
	;

NEWLINE
	: ('\r' '\n'? | '\n' ) -> skip
	;

COMMENT
	:'//' ~[\r\n]* -> skip
	;