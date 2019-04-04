#include "Bytecodes.hpp"

const char* Bytecode::bytecodeNames[] = {
        "NOP",
        "PUSH_CONSTANT",
        "PUSH_ARG",
        "PUSH_LOCAL",
        "POP",
        "POP_LOCAL",
        "DUP",
        "ADD",
        "SUB",
        "MUL",
        "DIV",
        "MOD",
        "JMP",
        "JMPE",
        "JMPL",
        "JMPG",
        "CALL",
        "RET",
        "PRINT_STRING",
        "PRINT_INT64",
        "CURRENT_TIME",
        "HALT",
        "ERROR"
};
