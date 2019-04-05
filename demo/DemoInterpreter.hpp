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
#include "JitBuilder.hpp"

#include "InterpreterTypeDictionary.hpp"

using OMR::JitBuilder::IlBuilder;
using OMR::JitBuilder::VirtualMachineStack;
using OMR::JitBuilder::VirtualMachineArray;
using OMR::JitBuilder::VirtualMachineRegister;
using OMR::JitBuilder::VirtualMachineState;
using OMR::JitBuilder::VirtualMachineRegisterInStruct;

#ifndef DEMO_INTERPRETER_INCL
#define DEMO_INTERPRETER_INCL

#define NOP_LENGTH 1
#define HALT_LENGTH 1
#define PUSH_CONSTANT_LENGTH 9
#define ADD_LENGTH 1
#define MUL_LENGTH 1
#define RETURN_LENGTH 1
#define PRINT_STRING_LENGTH 9
#define PRINT_INT64_LENGTH 1
#define JMP_LENGTH 9
#define JMPL_LENGTH 9
#define CALL_LENGTH 17
#define PUSH_LOCAL_LENGTH 9
#define POP_LOCAL_LENGTH 9
#define CURRENT_TIME_LENGTH 1
#define PUSH_ARG_LENGTH 9
#define DUP_LENGTH 1
#define SUB_LENGTH 1
#define POP_LENGTH 1

typedef int64_t (DemoInterpreterType)(VM *vm, Function *function, int64_t *a);

class DemoInterpreter: public OMR::JitBuilder::InterpreterBuilder {
public:
    DemoInterpreter(OMR::JitBuilder::TypeDictionary *types);
    virtual void Setup();

    static void defineFunctions(OMR::JitBuilder::RuntimeBuilder *rb, OMR::JitBuilder::TypeDictionary *types);
    static void registerHandlers(OMR::JitBuilder::RuntimeBuilder *rb);

private:
    static int64_t doNop(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doHalt(RuntimeBuilder *rb, IlBuilder *b);

    static int64_t doPushConstant(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doAdd(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doReturn(RuntimeBuilder *rb, IlBuilder *b);

    static int64_t doPrintString(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doPrintInt64(RuntimeBuilder *rb, IlBuilder *b);

    static int64_t doJMP(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doJMPL(RuntimeBuilder *rb, IlBuilder *b);

    static int64_t doCall(RuntimeBuilder *rb, IlBuilder *b);

    static int64_t doPushLocal(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doPopLocal(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doMul(RuntimeBuilder *rb, IlBuilder *b);

    static int64_t doCallWithJIT(RuntimeBuilder *rb, IlBuilder *b);

    static int64_t doCurrentTime(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doPushArg(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doDup(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doSub(RuntimeBuilder *rb, IlBuilder *b);
    static int64_t doPop(RuntimeBuilder *rb, IlBuilder *b);
};

class DemoVMState: public VirtualMachineState {
public:
    DemoVMState() :
        VirtualMachineState(),
            _stack(NULL),
            _stackTop(NULL),
            _locals(NULL),
            _localsBase(NULL),
            _args(NULL),
            _argsBase(NULL)
    {}

    DemoVMState(VirtualMachineStack *stack,
            VirtualMachineRegister *stackTop,
            VirtualMachineArray *locals,
            VirtualMachineRegister *localsBase,
            VirtualMachineArray *args,
            VirtualMachineRegister *argsBase) :
            VirtualMachineState(),
            _stack(stack),
            _stackTop(stackTop),
            _locals( locals),
            _localsBase(localsBase),
            _args(args),
            _argsBase(argsBase)
    {}

    virtual void Commit(IlBuilder *b) {
        _stack->Commit(b);
        _stackTop->Commit(b);

        _locals->Commit(b);
        _localsBase->Commit(b);

        _args->Commit(b);
        _argsBase->Commit(b);
    }

    virtual void Reload(IlBuilder *b) {
        _stackTop->Reload(b);
        _stack->Reload(b);

        _localsBase->Reload(b);
        _locals->Reload(b);

        _argsBase->Reload(b);
        _args->Reload(b);
    }

    virtual VirtualMachineState *MakeCopy() {
        DemoVMState *newState = new DemoVMState();
        newState->_stack = (VirtualMachineStack *) _stack->MakeCopy();
        newState->_stackTop = (VirtualMachineRegister *) _stackTop->MakeCopy();

        newState->_locals = (VirtualMachineArray *) _locals->MakeCopy();
        newState->_localsBase = (VirtualMachineRegister *) _localsBase->MakeCopy();

        newState->_args = (VirtualMachineArray *) _args->MakeCopy();
        newState->_argsBase = (VirtualMachineRegister *) _argsBase->MakeCopy();
        return newState;
    }

    virtual void MergeInto(VirtualMachineState *other, IlBuilder *b) {
        DemoVMState *otherState = (DemoVMState *) other;
        _stack->MergeInto(otherState->_stack, b);
        _stackTop->MergeInto(otherState->_stackTop, b);
        _locals->MergeInto(otherState->_locals, b);
        _localsBase->MergeInto(otherState->_localsBase, b);
        _args->MergeInto(otherState->_args, b);
        _argsBase->MergeInto(otherState->_argsBase, b);
    }

    VirtualMachineStack * _stack;
    VirtualMachineRegister * _stackTop;

    VirtualMachineArray * _locals;
    VirtualMachineRegister * _localsBase;

    VirtualMachineArray * _args;
    VirtualMachineRegister * _argsBase;
};

#endif //DEMO_INTERPRETER_INCL
