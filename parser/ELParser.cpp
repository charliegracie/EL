/*******************************************************************************
 * Copyright (c) 2016, 2018 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <map>

#include <inttypes.h>

#include "ELParser.hpp"

#define EYECATCHER "ELLE"
#define EYECATCHER_LENGTH 4

using namespace std;

#include "EL.hpp"
#include "Bytecodes.hpp"

ELParser::ELParser(const char * fileName) :
    _fileName(fileName),
    _program()
{}

ELParser::~ELParser() {
    if (_program.programName != NULL) {
        free(_program.programName);
        _program.programName = NULL;
    }

    Function **functions = _program.functions;
    if (functions != NULL) {
        for (int i = 0; i < _program.functionCount; i++) {
            if (NULL != functions[i]) {
                Function *function = functions[i];
                if (NULL != function->functionName) {
                    free(function->functionName);
                }
                if (NULL != function->opcodes) {
                    free(function->opcodes);
                }
                free(function);
            }
        }
        free(functions);
        _program.functionCount = 0;
        _program.functions = NULL;
    }

    String **strings = _program.strings;
    if (strings != NULL) {
        for (int i = 0; i < _program.stringCount; i++) {
            if (NULL != strings[i]) {
                free(strings[i]);
            }
        }
        free(strings);
        _program.stringCount = 0;
        _program.strings = NULL;
    }

    _infile.close();
}

bool ELParser::initialize() {
    _infile.open(_fileName, ios::binary | ios::in);
    if (!_infile.good()) {
        fprintf(stderr, "Error opening %s\n", _fileName);
        return false;
    }
    return true;
}

Program *ELParser::parseProgram() {
    if (0 != parseEyecatcher()) {
        fprintf(stderr, "Error parsing EL program file opening eyecatcher\n");
        return NULL;
    }

    int programNameLength = parseNameLength();
    char *programName = parseName(programNameLength);
    if (NULL == programName) {
        fprintf(stderr, "Malformed file. Can not parse program name\n");
        return NULL;
    }

    _program.programName = programName;

    // Parse functions
    int functionCount = parseFunctionCount();
    _program.functionCount = functionCount;

    int functionsSize = sizeof(Function) * functionCount;
    Function **functions = (Function **)malloc(functionsSize);
    if (NULL == functions) {
        fprintf(stderr, "Error allocating function array\n");
        return NULL;
    }
    memset(functions, 0, functionsSize);
    _program.functions = functions;

    for (int i = 0; i < functionCount; i++) {
        Function *newFunc = parseFunction();
        if (NULL == newFunc) {
            fprintf(stderr, "Error parsing function %d\n", i);
            return NULL;
        }
        functions[newFunc->functionID] = newFunc;
    }

    // Parse strings
    int64_t stringCount = parseInt64();
    int64_t stringSize = sizeof(String) * stringCount;
    String **strings = (String **)malloc(stringSize);
    if (NULL == strings) {
        fprintf(stderr, "Error allocating String array\n");
        return NULL;
    }
    _program.stringCount = stringCount;
    memset(strings, 0, stringSize);
    _program.strings = strings;

    for (int i = 0; i < stringCount; i++) {
        int64_t stringID = parseInt64();
        int64_t stringLength = parseInt64();
        String *newString = (String *)malloc(sizeof(String) + stringLength);
        if (NULL == newString) {
            fprintf(stderr, "Error allocating String\n");
            return NULL;
        }
        newString->length = stringLength;
        newString->data = (char*)((int8_t*)newString + sizeof(String));
        _infile.read((char *)newString->data, stringLength);

        _program.strings[stringID] = newString;
    }

    if (0 != parseEyecatcher()) {
        fprintf(stderr, "Error parsing EL program file closing eyecatcher\n");
        return NULL;
    }
    return &_program;
}

int ELParser::parseEyecatcher() {
    const char * eyecatcher = EYECATCHER;
    char buf[EYECATCHER_LENGTH];

    _infile.read((char *)&buf, sizeof(buf));
    for (int i = 0; i < EYECATCHER_LENGTH; i++) {
        if (eyecatcher[i] != buf[i]) {
            return -1;
        }
    }
    return 0;
}

int ELParser::parseNameLength() {
    int8_t buf[1];
    _infile.read((char *)&buf, sizeof(buf));

    return (int)buf[0];
}

char * ELParser::parseName(int nameLength) {
    char *buf = (char *)malloc(sizeof(char) * (nameLength + 1));
    if (NULL == buf) {
        return NULL;
    }
    _infile.read(buf, nameLength);
    buf[nameLength] = '\0';
    return buf;
}

int ELParser::parseFunctionCount() {
    int8_t buf[1];
    _infile.read((char *)&buf, sizeof(buf));

    return (int)buf[0];
}

Function * ELParser::parseFunction() {
    int functionNameLength = parseNameLength();
    char *functionName = parseName(functionNameLength);

    if (NULL == functionName) {
        fprintf(stderr, "error parsing function name\n");
        return NULL;
    }

    int64_t functionID = parseFunctionSize();
    int64_t argCount = parseFunctionSize();
    int64_t opcodeCount = parseFunctionSize();
    int64_t maxStackDepth = 0;
    int64_t localCount = 0;
    int8_t *opcodes = parseFunctionOpcodes(opcodeCount, &maxStackDepth, &localCount);

    if (NULL == opcodes) {
        free(functionName);
        return NULL;
    }

    Function *function = (Function *)malloc(sizeof(Function));
    if (NULL == function) {
        fprintf(stderr, "error allocating function\n");
        free(opcodes);
        free(functionName);
        return NULL;
    }

    function->functionName = functionName;
    function->functionID = functionID;
    function->maxStackDepth = maxStackDepth;
    function->argCount = argCount;
    function->localCount = localCount;
    function->opcodeCount = opcodeCount;
    function->opcodes = opcodes;
    function->compiledFunction = nullptr;
    function->invokedCount = 0;

    return function;
}

int64_t ELParser::parseFunctionSize() {
    int64_t val;
    _infile.read((char *)&val, sizeof(int64_t));
    convertToPlatformEndian(val);

    return val;
}

int64_t ELParser::parseInt64() {
    int64_t val;
    _infile.read((char *)&val, sizeof(int64_t));
    convertToPlatformEndian(val);

    return val;
}

int8_t *ELParser::parseFunctionOpcodes(int64_t opcodeCount, int64_t *functionMaxStackDepth, int64_t *localCount) {
    int64_t opcodeSize = opcodeCount * sizeof(int8_t);
    int8_t * opcodes = (int8_t *)malloc(opcodeSize);
    if (NULL == opcodes) {
        return NULL;
    }

    map<int64_t, int64_t> destinationStackSizes;

    int64_t currentStackDepth = 0;
    int64_t maxStackDepth = 0;
    int64_t maxLocalID = -1;
    int64_t index = 0;
    while (index < opcodeCount) {
        map<int64_t, int64_t>::iterator it = destinationStackSizes.find(index);
        if (it != destinationStackSizes.end()) {
            if (currentStackDepth != it->second) {
                if (currentStackDepth != -1) {
                    fprintf(stderr, "Mismatched stack size for instruction %" PRIu64 " that was a forward jump destination %" PRIu64 " != %" PRIu64 "\n", index, currentStackDepth, it->second);
                    free(opcodes);
                    return NULL;
                }
            }
            currentStackDepth = it->second;
        } else {
            if (currentStackDepth == -1) {
                fprintf(stderr, "Unreachable instruction after a return at index %" PRIu64 "\n", index);
                free(opcodes);
                return NULL;
            }
            destinationStackSizes.insert(make_pair(index, currentStackDepth));
        }

        _infile.read((char *)&opcodes[index], 1);
        Bytecodes opcode = (Bytecodes)opcodes[index++];

        switch(opcode) {
        case Bytecodes::NOP:
        {
            break;
        }
        case Bytecodes::PUSH_CONSTANT:
        {
            int64_t val;
            _infile.read((char *)&val, sizeof(int64_t));
            convertToPlatformEndian(val);
            write64(&opcodes[index], val);
            index += 8;
            currentStackDepth += 1;
            break;
        }
        case Bytecodes::PUSH_ARG:
        {
            int64_t val;
            _infile.read((char *)&val, sizeof(int64_t));
            convertToPlatformEndian(val);
            write64(&opcodes[index], val);
            index += 8;
            currentStackDepth += 1;
            break;
        }
        case Bytecodes::PUSH_LOCAL:
        {
            int64_t val;
            _infile.read((char *)&val, sizeof(int64_t));
            convertToPlatformEndian(val);
            if (val > maxLocalID) {
                maxLocalID = val;
            }
            write64(&opcodes[index], val);
            index += 8;
            currentStackDepth += 1;
            break;
        }
        case Bytecodes::POP:
            currentStackDepth -= 1;
            break;
        case Bytecodes::POP_LOCAL:
        {
            int64_t val;
            _infile.read((char *)&val, sizeof(int64_t));
            convertToPlatformEndian(val);
            if (val > maxLocalID) {
                maxLocalID = val;
            }
            write64(&opcodes[index], val);
            index += 8;
            currentStackDepth -= 1;
            break;
        }
        case Bytecodes::DUP:
            currentStackDepth += 1;
            break;
        case Bytecodes::ADD:
            currentStackDepth -= 1;
            break;
        case Bytecodes::SUB:
            currentStackDepth -= 1;
            break;
        case Bytecodes::MUL:
            currentStackDepth -= 1;
            break;
        case Bytecodes::DIV:
            currentStackDepth -= 1;
            break;
        case Bytecodes::MOD:
            currentStackDepth -= 1;
            break;
        case Bytecodes::JMP:
        {
            int64_t destination;
            _infile.read((char *)&destination, sizeof(int64_t));
            convertToPlatformEndian(destination);
            write64(&opcodes[index], destination);
            index += 8;

            pair<map<int64_t, int64_t>::iterator, bool> ret;
             ret = destinationStackSizes.insert(make_pair(destination, currentStackDepth));
             if (!ret.second) {
                 if (currentStackDepth != ret.first->second) {
                     fprintf(stderr, "Stack size mismatch (%" PRIu64 " != %" PRIu64 ") jumping to instruction %" PRIu64 "\n", currentStackDepth, ret.first->second, destination);
                     free(opcodes);
                     return NULL;
                 }
             }
             currentStackDepth = -1;
             break;
        }
        case Bytecodes::JMPE:
        case Bytecodes::JMPL:
        case Bytecodes::JMPG:
        {
            int64_t destination;
            _infile.read((char *)&destination, sizeof(int64_t));
            convertToPlatformEndian(destination);
            write64(&opcodes[index], destination);
            index += 8;

            currentStackDepth -= 2;
            pair<map<int64_t, int64_t>::iterator, bool> ret;
            ret = destinationStackSizes.insert(make_pair(destination, currentStackDepth));
            if (!ret.second) {
                if (currentStackDepth != ret.first->second) {
                    fprintf(stderr, "Stack size mismatch (%" PRIu64 " != %" PRIu64 ") jumping to instruction %" PRIu64 "\n", currentStackDepth, ret.first->second, destination);
                    free(opcodes);
                    return NULL;
                }
            }
            break;
        }
        case Bytecodes::CALL:
        {
            int64_t val;
            _infile.read((char *)&val, sizeof(int64_t));
            convertToPlatformEndian(val);
            write64(&opcodes[index], val);
            index += 8;

            _infile.read((char *)&val, sizeof(int64_t));
            convertToPlatformEndian(val);
            write64(&opcodes[index], val);
            index += 8;

            currentStackDepth -= (val - 1);
            break;
        }
        case Bytecodes::RET:
            if (currentStackDepth != 1) {
                fprintf(stderr, "Stack size %" PRIu64 " != 1 on return\n", currentStackDepth);
                free(opcodes);
                return NULL;
            }
            currentStackDepth = -1;
            break;
        case Bytecodes::PRINT_STRING:
        {
            int64_t val;
            _infile.read((char *)&val, sizeof(int64_t));
            convertToPlatformEndian(val);
            write64(&opcodes[index], val);
            index += 8;
            break;
        }
        case Bytecodes::PRINT_INT64:
        {
            currentStackDepth -= 1;
            break;
        }
        case Bytecodes::CURRENT_TIME:
        {
            currentStackDepth += 1;
            break;
        }
        case Bytecodes::HALT:
        {
            currentStackDepth = -1;
            break;
        }
        default:
            fprintf(stderr, "Unknown opcode at index %" PRIu64 " during load\n", index);
            free(opcodes);
            return NULL;
        }
        if (currentStackDepth > maxStackDepth) {
            maxStackDepth = currentStackDepth;
        }
    }
    *functionMaxStackDepth = maxStackDepth;
    *localCount = maxLocalID + 1;
    return opcodes;
}

inline int64_t ELParser::read64(int8_t *opcodes) {
    return *((int64_t *)opcodes);
}

inline void ELParser::write64(int8_t *opcodes, int64_t val) {
   *((int64_t *)opcodes) = val;
}
