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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <cstring>
#include <cstddef>

#include "EL.hpp"
#include "Bytecodes.hpp"

#ifndef CINTERPRETER_INCL
#define CINTERPRETER_INCL
//TODO use the real Bytecodes enum once computed goto is fixed
enum {
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
    HALT
};

class CInterpreter {
public:
    CInterpreter();
    int64_t interpret(VM *vm, Function *func, int64_t* args);

private:

    int64_t getImmediate(int8_t *opcodes, int64_t offset) {
        return *((int64_t *)((int8_t *)opcodes + offset));
    }
};

#endif /*CINTERPRETER_INCL */
