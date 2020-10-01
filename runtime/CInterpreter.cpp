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

#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "EL.hpp"

#include "Bytecodes.hpp"
#include "Helpers.hpp"
#include "CInterpreter.hpp"

#define PUSH(value) (*sp++ = value)
#define POP() (*--sp)
#define PEEK() (*(sp-1))

#if INTERP_FORCE_REGISTERS
#define REGISTER register
#define OPCODE_REG asm("%r15")
#define SP_REG asm("%r14")
#define LOCALS_REG asm("%r13")
#define ARGS_REG asm("%r12")
#else
#define REGISTER
#define OPCODE_REG
#define SP_REG
#define LOCALS_REG
#define ARGS_REG
#endif

#if INTERP_USE_COMPUTED_GOTO
#define InstructionEntry(name) &&lbl_##name
#define Instruction(name) lbl_##name
#define Next goto *(void *)tblArray[*opcodes]
#else
#define Instruction(name) case name
#define Next break
#endif

#define doNop() \
do { \
    opcodes += 1; \
} while (0)

#define doPushConstant() \
do { \
    PUSH(getImmediate(opcodes, IMMEDIATE0)); \
    opcodes += 9; \
} while (0)

#define doPushArg() \
do { \
    PUSH(args[getImmediate(opcodes, IMMEDIATE0)]); \
    opcodes += 9; \
} while (0)

#define doPushLocal() \
do { \
    PUSH(locals[getImmediate(opcodes, IMMEDIATE0)]); \
    opcodes += 9; \
} while (0)

#define doPop() \
do { \
    POP(); \
    opcodes += 1; \
} while (0)

#define doPopLocal() \
do { \
    locals[getImmediate(opcodes, IMMEDIATE0)] = POP(); \
    opcodes += 9; \
} while (0)

#define doDup() \
do { \
    int64_t val = PEEK(); \
    PUSH(val); \
    opcodes += 1; \
} while (0)

#define doAdd() \
do { \
    int64_t right = POP(); \
    int64_t left = POP(); \
    PUSH(left + right); \
    opcodes += 1; \
} while (0)

#define doSub() \
do { \
    int64_t right = POP(); \
    int64_t left = POP(); \
    PUSH(left - right); \
    opcodes += 1; \
} while (0)

#define doMul() \
do { \
    int64_t right = POP(); \
    int64_t left = POP(); \
    PUSH(left * right); \
    opcodes += 1; \
} while (0)

#define doDiv() \
do { \
    int64_t right = POP(); \
    int64_t left = POP(); \
    PUSH(left / right); \
    opcodes += 1; \
} while (0)

#define doMod() \
do { \
    int64_t right = POP(); \
    int64_t left = POP(); \
    PUSH(left % right); \
    opcodes += 1; \
} while (0)

#define doJMP() \
do { \
    int64_t jumpIndex = getImmediate(opcodes, IMMEDIATE0); \
    opcodes = &function->opcodes[jumpIndex]; \
} while(0)

#define doJMPE() \
do { \
    int64_t right = POP(); \
    int64_t left = POP(); \
    if (left == right) { \
        int64_t jumpIndex = getImmediate(opcodes, IMMEDIATE0); \
        opcodes = &function->opcodes[jumpIndex]; \
    } else { \
        opcodes += 9; \
    } \
} while(0)

#define doJMPL() \
do { \
    int64_t right = POP(); \
    int64_t left = POP(); \
    if (left < right) { \
        int64_t jumpIndex = getImmediate(opcodes, IMMEDIATE0); \
        opcodes = &function->opcodes[jumpIndex]; \
    } else { \
        opcodes += 9; \
    } \
} while(0)

#define doJMPG() \
do { \
    int64_t right = POP(); \
    int64_t left = POP(); \
    int64_t jumpIndex = getImmediate(opcodes, IMMEDIATE0); \
    if (left > right) { \
        opcodes = &function->opcodes[jumpIndex]; \
    } else { \
        opcodes += 9; \
    } \
} while(0)

#define doCall() \
do { \
    int64_t functionID = getImmediate(opcodes, IMMEDIATE0); \
    int64_t numberOfArgs = getImmediate(opcodes, IMMEDIATE1); \
    Function *toCall = vm->functions[functionID]; \
    int64_t *newArgs = sp - numberOfArgs; \
    frame->stack = sp; \
    CInterpreter interp; \
    int64_t ret = interp.interpret(vm, toCall, newArgs); \
    sp = newArgs; /*effectively popping the args off of s=the stack */\
    PUSH(ret); \
    opcodes += 17; \
} while(0)

#define doPrintString() \
do { \
    int64_t stringID = getImmediate(opcodes, IMMEDIATE0); \
    String *string = vm->strings[stringID]; \
    fprintf(stdout, "%.*s", (int32_t)string->length, string->data); \
    opcodes += 9; \
} while (0)

#define doPrintInt64() \
do { \
    fprintf(stdout, "%" PRIu64, POP()); \
    opcodes += 1; \
} while (0)

#define doCurrentTime() \
do { \
    struct timeval tp; \
    gettimeofday(&tp, NULL); \
    int64_t time = ((int64_t)tp.tv_sec) * 1000 + tp.tv_usec / 1000; \
    PUSH(time); \
    opcodes += 1; \
} while (0)

#define doHalt() \
do { \
    exit(0); \
} while (0)

CInterpreter::CInterpreter() {}

int64_t CInterpreter::interpret(VM *vm, Function *function, int64_t *a) {
    Frame f;
    Frame *frame = &f;
    int64_t stackSize = function->maxStackDepth * sizeof(int64_t);
    int64_t localsSize = function->localCount * sizeof(int64_t);
    int64_t *data = nullptr;
    if (function->maxStackDepth + function->localCount <= FRAME_INLINED_DATA_LENGTH) {
        frame->stack = f.inlinedData;
        frame->locals = frame->stack + function->maxStackDepth;
    } else {
        data = allocateFrameData(function, stackSize, localsSize);
        frame->stack = data;
        frame->locals = (int64_t*)((int8_t*)data + stackSize);
    }
    frame->args = a;

    frame->previous = vm->frame;
    vm->frame = frame;

    REGISTER int8_t *opcodes OPCODE_REG = function->opcodes;
    REGISTER int64_t *sp SP_REG = frame->stack;
    REGISTER int64_t *locals LOCALS_REG = frame->locals;
    REGISTER int64_t *args ARGS_REG = frame->args;

#if INTERP_USE_COMPUTED_GOTO
    static const void * const tblArray[] = {
            InstructionEntry(NOP),
            InstructionEntry(PUSH_CONSTANT),
            InstructionEntry(PUSH_ARG),
            InstructionEntry(PUSH_LOCAL),
            InstructionEntry(POP),
            InstructionEntry(POP_LOCAL),
            InstructionEntry(DUP),
            InstructionEntry(ADD),
            InstructionEntry(SUB),
            InstructionEntry(MUL),
            InstructionEntry(DIV),
            InstructionEntry(MOD),
            InstructionEntry(JMP),
            InstructionEntry(JMPE),
            InstructionEntry(JMPL),
            InstructionEntry(JMPG),
            InstructionEntry(CALL),
            InstructionEntry(RET),
            InstructionEntry(PRINT_STRING),
            InstructionEntry(PRINT_INT64),
            InstructionEntry(CURRENT_TIME),
            InstructionEntry(HALT)
    };
    goto *tblArray[*opcodes];
#else
    while (true) {
        switch(*opcodes) {
#endif
        Instruction(NOP):
        {
            doNop();
            Next;
        }
        Instruction(PUSH_CONSTANT):
        {
            doPushConstant();
            Next;
        }
        Instruction(PUSH_ARG):
        {
            doPushArg();
            Next;
        }
        Instruction(PUSH_LOCAL):
        {
            doPushLocal();
            Next;
        }
        Instruction(POP):
        {
            doPop();
            Next;
        }
        Instruction(POP_LOCAL):
        {
            doPopLocal();
            Next;
        }
        Instruction(DUP):
        {
            doDup();
            Next;
        }
        Instruction(ADD):
        {
            doAdd();
            Next;
        }
        Instruction(SUB):
        {
            doSub();
            Next;
        }
        Instruction(MUL):
        {
            doMul();
            Next;
        }
        Instruction(DIV):
        {
            doDiv();
            Next;
        }
        Instruction(MOD):
        {
            doMod();
            Next;
        }
        Instruction(JMP):
        {
            doJMP();
            Next;
        }
        Instruction(JMPE):
        {
            doJMPE();
            Next;
        }
        Instruction(JMPL):
        {
            doJMPL();
            Next;
        }
        Instruction(JMPG):
        {
            doJMPG();
            Next;
        }
        Instruction(CALL):
        {
            doCall();
            Next;
        }
        Instruction(RET):
        {
            int64_t retVal = POP();
            vm->frame = frame->previous;
            if (nullptr != data) {
                freeFrameData(data);
            }
            return retVal;
        }
        Instruction(PRINT_STRING):
        {
            doPrintString();
            Next;
        }
        Instruction(PRINT_INT64):
        {
            doPrintInt64();
            Next;
        }
        Instruction(CURRENT_TIME):
        {
            doCurrentTime();
            Next;
        }
        Instruction(HALT):
        {
            doHalt();
        }
#if INTERP_USE_COMPUTED_GOTO==0
        default:
            fprintf(stderr, "Unknown opcode  %d during execution. Exiting...\n", *opcodes);
            exit(-1);
        }
    }
#endif
    /* unreachable*/
}
