#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include "EL.hpp"
#include "ELParser.hpp"
#include "InterpreterTypeDictionary.hpp"
#include "DemoInterpreter.hpp"
#include "Bytecodes.hpp"
#include "Helpers.hpp"

using namespace std;

Function *findMainFunction(Program *program);
int64_t read64(int8_t *opcodes);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: demo <filename>\n");
        exit(-1);
    }

    ELParser parser(argv[1]);

    if (!parser.initialize()) {
        return -1;
    }

    Program *program = parser.parseProgram();
    if (NULL == program) {
        return -2;
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

        initializeJit();
        InterpreterTypeDictionary types;
        DemoInterpreter method(&types);
        void *entry = 0;
        int32_t rc = compileMethodBuilder(&method, &entry);
        if (0 == rc) {
            DemoInterpreterType *demo_interpret = (DemoInterpreterType *)entry;
            vm.interpretFunction = (void *)demo_interpret;
            ret = demo_interpret(&vm, main, nullptr);
        } else {
            fprintf(stderr, "Error generating DemoInterpreter %d\n", rc);
        }
        shutdownJit();

        fprintf(stdout, "Main returned %lld\n", ret);
    } else {
        fprintf(stderr, "Failed to find main function\n");
    }

    return 0;
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

int64_t read64(int8_t *opcodes) {
    return *((int64_t *)opcodes);
}

