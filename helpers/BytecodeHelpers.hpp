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

#include "JitBuilder.hpp"
#include "EL.hpp"

using OMR::JitBuilder::IlType;
using OMR::JitBuilder::IlValue;
using OMR::JitBuilder::IlBuilder;
using OMR::JitBuilder::RuntimeBuilder;
using OMR::JitBuilder::TypeDictionary;
using OMR::JitBuilder::VirtualMachineRegister;
using OMR::JitBuilder::VirtualMachineRegisterInStruct;
using OMR::JitBuilder::VirtualMachineArray;
using OMR::JitBuilder::VirtualMachineStack;
using OMR::JitBuilder::VirtualMachineState;

using OMR::JitBuilder::VirtualMachineOperandArray;
using OMR::JitBuilder::VirtualMachineOperandStack;

void push(RuntimeBuilder *rb, IlBuilder *builder, IlValue *value);
IlValue *pop(RuntimeBuilder *rb, IlBuilder *builder);
IlValue *runtime_pop(RuntimeBuilder *rb, IlBuilder *builder);
IlValue *peek(RuntimeBuilder *rb, IlBuilder *builder);
void dup(RuntimeBuilder *rb, IlBuilder *builder);

IlValue *getArg(RuntimeBuilder *rb, IlBuilder *builder, IlValue *argIndex);

IlValue *getLocal(RuntimeBuilder *rb, IlBuilder *builder, IlValue *localIndex);
void setLocal(RuntimeBuilder *rb, IlBuilder *builder, IlValue *localIndex, IlValue *value);

int64_t invokedCompiledFunction(VM *vm, Function *function, int64_t*args);
void compileFunction(VM *vm, Function *function);

int64_t registerNop(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerPushConstant(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerPushArg(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerPushLocal(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerPop(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerPopLocal(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerDup(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerAdd(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerSub(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerMul(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerDiv(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerMod(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerJMP(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerJMPE(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerJMPL(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerJMPG(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerCall(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerRet(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerPrintString(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerPrintInt64(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerCurrentTime(RuntimeBuilder *rb, IlBuilder *b);
int64_t registerHalt(RuntimeBuilder *rb, IlBuilder *b);

class InterpreterVMState: public VirtualMachineState {
public:
    InterpreterVMState() :
            VirtualMachineState(),
            _stack(NULL),
            _stackTop(NULL),
            _locals(NULL),
            _localsBase(NULL),
            _args(NULL),
            _argsBase(NULL)
    {}

    InterpreterVMState(VirtualMachineStack *stack,
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
        InterpreterVMState *newState = new InterpreterVMState();
        newState->_stack = (VirtualMachineStack *) _stack->MakeCopy();
        newState->_stackTop = (VirtualMachineRegister *) _stackTop->MakeCopy();

        newState->_locals = (VirtualMachineArray *) _locals->MakeCopy();
        newState->_localsBase = (VirtualMachineRegister *) _localsBase->MakeCopy();

        newState->_args = (VirtualMachineArray *) _args->MakeCopy();
        newState->_argsBase = (VirtualMachineRegister *) _argsBase->MakeCopy();
        return newState;
    }

    virtual void MergeInto(VirtualMachineState *other, IlBuilder *b) {
        InterpreterVMState *otherState = (InterpreterVMState *) other;
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

