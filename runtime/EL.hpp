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
#include <stdarg.h>
#include <cstring>
#include <cstddef>

#include "ELConfig.h"

#ifndef EL_INCL
#define EL_INCL

/* Defines that control how the different EL Interpreters are built */
/* CInterpreter specific defines */
#define INTERP_FORCE_REGISTERS 1
#define INTERP_USE_COMPUTED_GOTO 1

/* JitBuilder specific defines */
#define USE_COMPUTED_GOTO 1

#define FRAME_INLINED_DATA_LENGTH 8
#define INVOCATIONS_BEFORE_COMPILE 10
//#define INVOCATIONS_BEFORE_COMPILE 200000000

#define IMMEDIATE0 1
#define IMMEDIATE1 9

typedef struct Function {
    char *functionName;
    int64_t functionID;
    int64_t invokedCount;
    void *compiledFunction;
    int64_t maxStackDepth;
    int64_t argCount;
    int64_t localCount;
    int64_t opcodeCount;
    int8_t *opcodes;
} Function;

typedef struct Frame {
    Frame *previous;
    Function *function;
    int64_t *stack;
    int64_t *args;
    int64_t *locals;
    int64_t inlinedData[FRAME_INLINED_DATA_LENGTH];
} Frame;

typedef struct String {
    int64_t length;
    char *data;
} String;

typedef struct VM {
    Function **functions;
    String **strings;
    Frame *frame;
    void *interpretFunction;
    int64_t verbose;
} VM;

typedef struct Program {
    char *programName;
    int64_t functionCount;
    Function **functions;
    int64_t stringCount;
    String **strings;
} Program;

#endif /* EL_INCL */
