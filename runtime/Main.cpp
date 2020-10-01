#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <inttypes.h>

#include "EL.hpp"
#include "ELParser.hpp"
#include "CInterpreter.hpp"
#include "IBInterpreter.hpp"
#include "InterpreterTypeDictionary.hpp"
#include "JBInterpreter.hpp"
#include "Bytecodes.hpp"
#include "Helpers.hpp"

typedef struct Options {
    const char *programFileName;
    bool dumpProgram;
    bool debugExecution;
    bool parseOnly;
    int64_t interpreterType;
} Options;

using namespace std;

void setDefaultOptions(Options *options);
int64_t parseOptions(Options *options, int argc, char *argv[]);
Function *findMainFunction(Program *program);
void dumpProgram(Program *program);
int64_t read64(int8_t *opcodes);

int main(int argc, char *argv[]) {
    Options options;
    setDefaultOptions(&options);
    if (parseOptions(&options, argc, argv) != 0) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "\treader [options] programFile\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "\t-it<0,1,2>\tChoose the interpreter to use. default 0\n");
        fprintf(stderr, "\t-o\tDump program after loading\n");
        fprintf(stderr, "\t-l\tOnly load the program but do not execute it\n");
        fprintf(stderr, "\t-t\tTrace the runtime execution\n");
        return -1;
    }

    ELParser parser(options.programFileName);

    if (!parser.initialize()) {
        return -1;
    }

    Program *program = parser.parseProgram();
    if (NULL == program) {
        return -2;
    }

    if (options.dumpProgram) {
        dumpProgram(program);
    }

    if (options.parseOnly) {
        fprintf(stdout, "Finished parsing program \"%s\" with no errors\n", program->programName);
        return 0;
    }

    Function *main = findMainFunction(program);
    if (NULL != main) {
        VM vm;
        vm.functions = program->functions;
        vm.strings = program->strings;
        vm.interpretFunction = nullptr;
        vm.frame = nullptr;
        vm.verbose = 1;
        int64_t ret = -1;
        if (options.interpreterType == 0) {
            CInterpreter interp;
            ret = interp.interpret(&vm, main, nullptr);
        } else if(options.interpreterType == 1) {
            initializeJit();
            InterpreterTypeDictionary types;
            JBInterpreter method(&types);
            void *entry = 0;
            int32_t rc = compileMethodBuilder(&method, &entry);
            if (0 == rc) {
                JBInterpreterType *jb_interpret = (JBInterpreterType *)entry;
                ret = jb_interpret(&vm, main, nullptr);
            } else {
                fprintf(stderr, "Error generating JBInterpreter %d\n", rc);
            }
            shutdownJit();
        } else if(options.interpreterType == 2) {
            initializeJit();
            InterpreterTypeDictionary types;
            IBInterpreter method(&types);
            void *entry = 0;
            int32_t rc = compileMethodBuilder(&method, &entry);
            if (0 == rc) {
                IBInterpreterType *ib_interpret = (IBInterpreterType *)entry;
                vm.interpretFunction = (void *)ib_interpret;
                ret = ib_interpret(&vm, main, nullptr);
            } else {
                fprintf(stderr, "Error generating IBInterpreter %d\n", rc);
            }
            shutdownJit();
        } else {
            fprintf(stderr, "Error unknown interpreter type %" PRIu64 "\n", options.interpreterType);
            return -3;
        }
        fprintf(stdout, "Main returned %" PRIu64 "\n", ret);
    } else {
        fprintf(stderr, "Failed to find main function\n");
    }

    return 0;
}

int64_t parseOptions(Options *options, int argc, char *argv[]) {
    options->programFileName = (const char *)argv[argc - 1];
    for (int i = 1; i < argc - 1; i++) {
        char *arg = argv[i];
        if (0 == strcmp("-o", arg)) {
            options->dumpProgram = true;
        } else if (0 == strcmp("-l", arg)) {
            options->parseOnly = true;
            if (options->debugExecution) {
                fprintf(stderr, "Invalid option combination. -t and -l can not be used together\n");
                return -1;
            }
        } else if (0 == strcmp("-t", arg)) {
            options->debugExecution = true;
            if (options->parseOnly) {
                fprintf(stderr, "Invalid option combination. -t and -l can not be used together\n");
                return -1;
            }
        } else if (0 == strcmp("-it", arg)) {
            options->interpreterType = atol(argv[++i]);
            fprintf(stderr, "type %" PRIu64 "\n", options->interpreterType);
        }  else {
            fprintf(stderr, "Invalid option %s\n", arg);
            return -1;
        }
    }
    return 0;
}

void setDefaultOptions(Options *options) {
    options->programFileName = NULL;
    options->debugExecution = false;
    options->dumpProgram = false;
    options->parseOnly = false;
    options->interpreterType = 0;
}

Function *findMainFunction(Program *program) {
    Function **functions = program->functions;
    int functionCount = program->functionCount;
    for (int i = 0; i < functionCount; i++) {
        if (0 == strcmp("main", functions[i]->functionName)) {
            return functions[i];
        }
    }
    return NULL;
}

void dumpProgram(Program *program) {
    fprintf(stdout, "Dumping Program: %s\n", program->programName);
    for (int i = 0; i < program->functionCount; i++) {
        Function * function = program->functions[i];
        char *functionName = function->functionName;
        int64_t opcodeCount = function->opcodeCount;
        int8_t *opcodes = function->opcodes;

        fprintf(stdout, "Function: %s opcodeCount %" PRIu64 "\n", functionName, opcodeCount);

        int64_t index = 0;
        while (index < opcodeCount) {
            Bytecodes opcode = (Bytecodes)opcodes[index];
            fprintf(stdout, "\t%" PRIu64, index);
            switch (opcode) {
            case Bytecodes::NOP:
                fprintf(stdout, "\tNOP\n");
                index += 1;
                break;
            case Bytecodes::PUSH_CONSTANT:
                fprintf(stdout, "\tPUSH_CONSTANT %" PRIu64 "\n", read64(opcodes + index + 1));
                index += 9;
                break;
            case Bytecodes::PUSH_ARG:
                fprintf(stdout, "\tPUSH_ARG %" PRIu64 "\n", read64(opcodes + index + 1));
                index += 9;
                break;
            case Bytecodes::PUSH_LOCAL:
                fprintf(stdout, "\tPUSH_LOCAL %" PRIu64 "\n", read64(opcodes + index + 1));
                index += 9;
                break;
            case Bytecodes::POP:
                fprintf(stdout, "\tPOP\n");
                index += 1;
                break;
            case Bytecodes::POP_LOCAL:
                fprintf(stdout, "\tPOP_LOCAL %" PRIu64 "\n", read64(opcodes + index + 1));
                index += 9;
                break;
            case Bytecodes::DUP:
                fprintf(stdout, "\tDUP\n");
                index += 1;
                break;
            case Bytecodes::ADD:
                fprintf(stdout, "\tADD\n");
                index += 1;
                break;
            case Bytecodes::SUB:
                fprintf(stdout, "\tSUB\n");
                index += 1;
                break;
            case Bytecodes::MUL:
                fprintf(stdout, "\tMUL\n");
                index += 1;
                break;
            case Bytecodes::DIV:
                fprintf(stdout, "\tDIV\n");
                index += 1;
                break;
            case Bytecodes::MOD:
                fprintf(stdout, "\tMOD\n");
                index += 1;
                break;
            case Bytecodes::JMP:
                fprintf(stdout, "\tJMP %" PRIu64 "\n", read64(opcodes + index + 1));
                index += 9;
                break;
            case Bytecodes::JMPE:
                fprintf(stdout, "\tJMPE %" PRIu64 "\n", read64(opcodes + index + 1));
                index += 9;
                break;
            case Bytecodes::JMPL:
                fprintf(stdout, "\tJMPL %" PRIu64 "\n", read64(opcodes + index + 1));
                index += 9;
                break;
            case Bytecodes::JMPG:
                fprintf(stdout, "\tJMPG %" PRIu64 "\n", read64(opcodes + index + 1));
                index += 9;
                break;
            case Bytecodes::CALL:
            {
                int64_t functionID = read64(opcodes + index + 1);
                fprintf(stdout, "\tCALL %s(%" PRIu64 ") %" PRIu64 "\n", program->functions[functionID]->functionName, functionID, read64(opcodes + index + 9));
                index += 17;
                break;
            }
            case Bytecodes::RET:
                fprintf(stdout, "\tRET\n");
                index += 1;
                break;
            case Bytecodes::PRINT_STRING:
            {
                int64_t stringID = read64(opcodes + index + 1);
                String *str = program->strings[stringID];
                string temp = str->data;

                string newLine("\\n");
                size_t pos = temp.find('\n');
                while (pos != std::string::npos) {
                    temp = temp.replace(pos, 1, newLine);
                    pos = temp.find('\n');
                }

                fprintf(stdout, "\tPRINT_STRING \"%.*s\"\n", (int32_t)temp.length(), temp.data());
                index += 9;
                break;
            }
            case Bytecodes::PRINT_INT64:
            {
                fprintf(stdout, "\tPRINT_INT64\n");
                index += 1;
                break;
            }
            case Bytecodes::CURRENT_TIME:
            {
                fprintf(stdout, "\tCURRENT_TIME\n");
                index += 1;
                break;
            }
            case Bytecodes::HALT:
            {
                fprintf(stdout, "\tHALT\n");
                index += 1;
                break;
            }
            default:
                fprintf(stderr, "\tUnknown opcode at %" PRIu64 ". Exiting...\n", index);
                exit(-1);
            }
        }
    }
}

int64_t read64(int8_t *opcodes) {
    return *((int64_t *)opcodes);
}

