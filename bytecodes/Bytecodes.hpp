/*******************************************************************************
 * Copyright (c) 2019, 2019 IBM Corp. and others
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

#include <string>
#include <stdint.h>

#ifndef BYTECODES_INCL
#define BYTECODES_INCL

enum class Bytecodes : int8_t {
    NOP,
    PUSH_CONSTANT,
    PUSH_ARG,
    PUSH_LOCAL,
    POP,
    POP_LOCAL,
    DUP,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    JMP,
    JMPE,
    JMPL,
    JMPG,
    CALL,
    RET,
    PRINT_STRING,
    PRINT_INT64,
    CURRENT_TIME,
    HALT,
    ERROR
};

class Bytecode {
public:
    static char* getBytecodeName(Bytecodes bytecode) {
        return (char *)bytecodeNames[static_cast<int8_t>(bytecode)];
    }
    static Bytecodes getBytecode(std::string name) {
        if (0 == name.compare(getBytecodeName(Bytecodes::NOP))) {
            return Bytecodes::NOP;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::PUSH_CONSTANT))) {
            return Bytecodes::PUSH_CONSTANT;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::PUSH_ARG))) {
            return Bytecodes::PUSH_ARG;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::PUSH_LOCAL))) {
            return Bytecodes::PUSH_LOCAL;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::POP))) {
            return Bytecodes::POP;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::POP_LOCAL))) {
            return Bytecodes::POP_LOCAL;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::DUP))) {
            return Bytecodes::DUP;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::ADD))) {
            return Bytecodes::ADD;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::SUB))) {
            return Bytecodes::SUB;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::MUL))) {
            return Bytecodes::MUL;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::DIV))) {
            return Bytecodes::DIV;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::MOD))) {
            return Bytecodes::MOD;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::JMP))) {
            return Bytecodes::JMP;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::JMPE))) {
            return Bytecodes::JMPE;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::JMPL))) {
            return Bytecodes::JMPL;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::JMPG))) {
            return Bytecodes::JMPG;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::RET))) {
            return Bytecodes::RET;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::CALL))) {
            return Bytecodes::CALL;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::PRINT_STRING))) {
            return Bytecodes::PRINT_STRING;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::PRINT_INT64))) {
            return Bytecodes::PRINT_INT64;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::CURRENT_TIME))) {
            return Bytecodes::CURRENT_TIME;
        } else if (0 == name.compare(getBytecodeName(Bytecodes::HALT))) {
            return Bytecodes::HALT;
        } else {
            return Bytecodes::ERROR;
        }
    }
    static int32_t getBytecodeAsInt(Bytecodes bytecode) {
        return static_cast<int32_t>(bytecode);
    }

private:
    static const char* bytecodeNames[];
};

#endif /* BYTECODES_INCL */

