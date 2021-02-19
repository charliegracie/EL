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

#include <time.h>
#include <sys/time.h>

#include "JitBuilder.hpp"
#include "BytecodeHelpers.hpp"

#include "InterpreterTypeDictionary.hpp"
#include "CMInterpreterMethod.hpp"

#include "EL.hpp"

using OMR::JitBuilder::IlType;
using OMR::JitBuilder::IlValue;
using OMR::JitBuilder::IlBuilder;
using OMR::JitBuilder::RuntimeBuilder;
using OMR::JitBuilder::TypeDictionary;
using OMR::JitBuilder::VirtualMachineRegister;
using OMR::JitBuilder::VirtualMachineRegisterInStruct;
using OMR::JitBuilder::VirtualMachineOperandStack;

void push(RuntimeBuilder *rb, IlBuilder *builder, IlValue *value) {
    InterpreterVMState *state = (InterpreterVMState *)rb->GetVMState(builder);
    state->_stack->Push(builder, value);
}

IlValue *pop(RuntimeBuilder *rb, IlBuilder *builder) {
    InterpreterVMState *state = (InterpreterVMState *)rb->GetVMState(builder);
    return state->_stack->Pop(builder);
}

IlValue *peek(RuntimeBuilder *rb, IlBuilder *builder) {
    InterpreterVMState *state = (InterpreterVMState *)rb->GetVMState(builder);
    return state->_stack->Top(builder);
}

void dup(RuntimeBuilder *rb, IlBuilder *builder) {
    InterpreterVMState *state = (InterpreterVMState *)rb->GetVMState(builder);
    state->_stack->Dup(builder);
}

IlValue *getArg(RuntimeBuilder *rb, IlBuilder *builder, IlValue *argIndex) {
    InterpreterVMState *state = (InterpreterVMState *)rb->GetVMState(builder);
    return state->_args->Get(builder, argIndex);
}

IlValue *getLocal(RuntimeBuilder *rb, IlBuilder *builder, IlValue *localIndex) {
    InterpreterVMState *state = (InterpreterVMState *)rb->GetVMState(builder);
    return state->_locals->Get(builder, localIndex);
}

void setLocal(RuntimeBuilder *rb, IlBuilder *builder, IlValue *localIndex, IlValue *value) {
    InterpreterVMState *state = (InterpreterVMState *)rb->GetVMState(builder);
    state->_locals->Set(builder, localIndex, value);
}

void compileFunction(VM *vm, Function *function) {
    InterpreterTypeDictionary types;
    CMInterpreterMethod method(&types, vm, function);
    void *entry = 0;
    if (vm->verbose) {
        fprintf(stderr, "Attempting to compile %s\n", function->functionName);
    }
    int32_t rc = compileMethodBuilder(&method, &entry);
    if (0 == rc) {
        if (vm->verbose) {
            fprintf(stderr, "Successfully compiled %s\n", function->functionName);
        }
        function->compiledFunction = (void *)entry;
    }
}

int64_t registerNop(RuntimeBuilder *rb, IlBuilder *b)
   {
   rb->DefaultFallthrough(b, b->ConstInt64(1));
   return 0;
   }

int64_t registerPushConstant(RuntimeBuilder *rb, IlBuilder *b)
   {
   IlValue *constant = rb->GetInt64Immediate(b, b->ConstInt64(1));
   push(rb, b, constant);
   rb->DefaultFallthrough(b, b->ConstInt64(9));
   return 0;
   }

int64_t registerPushArg(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *argIndex = rb->GetInt64Immediate(b, b->ConstInt64(1));
    IlValue *arg = getArg(rb, b, argIndex);
    push(rb, b, arg);
    rb->DefaultFallthrough(b, b->ConstInt64(9));
    return 0;
}

int64_t registerPushLocal(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *localIndex = rb->GetInt64Immediate(b, b->ConstInt64(1));
    IlValue *local = getLocal(rb, b, localIndex);
    push(rb, b, local);
    rb->DefaultFallthrough(b, b->ConstInt64(9));
    return 0;
}

int64_t registerPop(RuntimeBuilder *rb, IlBuilder *b) {
    pop(rb, b);
    rb->DefaultFallthrough(b, b->ConstInt64(1));
    return 0;
}

int64_t registerPopLocal(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *localIndex = rb->GetInt64Immediate(b, b->ConstInt64(1));
    IlValue *local = pop(rb, b);
    setLocal(rb, b, localIndex, local);
    rb->DefaultFallthrough(b, b->ConstInt64(9));
    return 0;
}

int64_t registerDup(RuntimeBuilder *rb, IlBuilder *b) {
    dup(rb, b);
    rb->DefaultFallthrough(b, b->ConstInt64(1));
    return 0;
}

int64_t registerAdd(RuntimeBuilder *rb, IlBuilder *b) {
    //TODO switch all IlValue * to named tmps for all operations
    IlValue *right = pop(rb, b);
    IlValue *left = pop(rb, b);
    IlValue *res = b->Add(left, right);
    push(rb, b, res);
    rb->DefaultFallthrough(b, b->ConstInt64(1));
    return 0;
}

int64_t registerSub(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *right = pop(rb, b);
    IlValue *left = pop(rb, b);
    IlValue *res = b->Sub(left, right);
    push(rb, b, res);
    rb->DefaultFallthrough(b, b->ConstInt64(1));
    return 0;
}

int64_t registerMul(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *right = pop(rb, b);
    IlValue *left = pop(rb, b);
    IlValue *res = b->Mul(left, right);
    push(rb, b, res);
    rb->DefaultFallthrough(b, b->ConstInt64(1));
    return 0;
}

int64_t registerDiv(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *right = pop(rb, b);
    IlValue *left = pop(rb, b);
    IlValue *res = b->Div(left, right);
    push(rb, b, res);
    rb->DefaultFallthrough(b, b->ConstInt64(1));
    return 0;
}

int64_t registerMod(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *right = pop(rb, b);
    IlValue *left = pop(rb, b);
    IlValue *res = b->Rem(left, right);
    push(rb, b, res);
    rb->DefaultFallthrough(b, b->ConstInt64(1));
    return 0;
}

int64_t registerJMP(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *target = rb->GetInt64Immediate(b, b->ConstInt64(1));
    rb->Jump(b, target, true);
    return 0;
}

int64_t registerJMPE(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *right = pop(rb, b);
    IlValue *left = pop(rb, b);
    IlValue *condition = b->EqualTo(left, right);
    IlValue *target = rb->GetInt64Immediate(b, b->ConstInt64(1));
    rb->JumpIfOrFallthrough(b, condition, target, b->ConstInt64(9), true);
    return 0;
}

int64_t registerJMPL(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *right = pop(rb, b);
    IlValue *left = pop(rb, b);
    IlValue *condition = b->LessThan(left, right);
    IlValue *target = rb->GetInt64Immediate(b, b->ConstInt64(1));
    rb->JumpIfOrFallthrough(b, condition, target, b->ConstInt64(9), true);
    return 0;
}

int64_t registerJMPG(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *right = pop(rb, b);
    IlValue *left = pop(rb, b);
    IlValue *condition = b->GreaterThan(left, right);
    IlValue *target = rb->GetInt64Immediate(b, b->ConstInt64(1));
    rb->JumpIfOrFallthrough(b, condition, target, b->ConstInt64(9), true);
    return 0;
}

int64_t registerCall(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *functionID = rb->GetInt64Immediate(b, b->ConstInt64(1));
    IlValue *argCount = rb->GetInt64Immediate(b, b->ConstInt64(9));

    b->Store("newFunction",
    b->     LoadAt(b->typeDictionary()->PointerTo(b->typeDictionary()->LookupStruct("Function")),
    b->           IndexAt(b->typeDictionary()->PointerTo(b->typeDictionary()->PointerTo(b->typeDictionary()->LookupStruct("Function"))),
    b->                   LoadIndirect("VM", "functions", b->Load("vm")), functionID)));

    b->Store("invokedCount",
    b->     LoadIndirect("Function", "invokedCount",
    b->                 Load("newFunction")));

    IlValue *incrementCondition = b->LessThan(
                                  b->       Load("invokedCount"),
                                  b->       ConstInt64(INVOCATIONS_BEFORE_COMPILE));

    IlBuilder *incrementBuilder = nullptr;
    b->IfThen(&incrementBuilder, incrementCondition);

    incrementBuilder->Store("invokedCount",
    incrementBuilder->     Add(
    incrementBuilder->        Load("invokedCount"),
    incrementBuilder->        ConstInt64(1)));

    incrementBuilder->StoreIndirect("Function", "invokedCount",
    incrementBuilder->              Load("newFunction"),
    incrementBuilder->              Load("invokedCount"));

    IlValue *compileCondition = incrementBuilder->EqualTo(
                                incrementBuilder->       Load("invokedCount"),
                                incrementBuilder->       ConstInt64(INVOCATIONS_BEFORE_COMPILE));

    IlBuilder *compileBuilder = nullptr;
    incrementBuilder->IfThen(&compileBuilder, compileCondition);

    compileBuilder->Call("compileFunction", 2,
    compileBuilder->    Load("vm"),
    compileBuilder->    Load("newFunction"));

    b->Store("compiledFunction",
    b->     LoadIndirect("Function", "compiledFunction",
    b->                 Load("newFunction")));

    b->Store("argCountSize",
    b->        Mul(
    b->           ConstInt64(sizeof(int64_t)),
                  argCount));

    InterpreterVMState *state = (InterpreterVMState *)rb->GetVMState(b);
    state->Commit(b);
    b->Store("newArgs",
    b->     Sub(
               state->_stackTop->Load(b),
    b->        Load("argCountSize")));

    b->Store("callJit",
    b->     EqualTo(
    b->            Load("compiledFunction"),
    b->            NullAddress()));

    IlBuilder *callJit = nullptr;
    IlBuilder *callInterpreter = nullptr;
    b->IfThenElse(&callInterpreter, &callJit,
    b->          Load("callJit"));

    callJit->Store("call_retVal",
    callJit->     ComputedCall("invokeCompiledFunction", 3, callJit->Load("compiledFunction"), callJit->Load("vm"), callJit->Load("newArgs")));

//    callJit->Store("call_retVal",
//    callJit->     Call("invokedCompiledFunction", 3, callJit->Load("vm"), callJit->Load("newFunction"), callJit->Load("newArgs")));

    callInterpreter->Store("call_retVal",
    callInterpreter->     Call("ib_interpret", 3, callInterpreter->Load("vm"), callInterpreter->Load("newFunction"), callInterpreter->Load("newArgs")));

    state->_stack->Drop(b, rb->GetInt64Immediate(b, b->ConstInt64(9)));

    push(rb, b, b->Load("call_retVal"));
    rb->DefaultFallthrough(b, b->ConstInt64(17));
    return 0;
}

int64_t registerRet(RuntimeBuilder *rb, IlBuilder *b)
   {
   b->Store("return_retVal", pop(rb, b));
   b->StoreIndirect("VM", "frame",
   b->             Load("vm"),
   b->             LoadIndirect("Frame", "previous",
   b->                         Load("frame")));

   IlBuilder *freeFrame = nullptr;
   b->IfThen(&freeFrame,
   b->       NotEqualTo(
   b->                 Load("frameData"),
   b->                 NullAddress()));

   freeFrame->Call("freeFrameData", 1, freeFrame->Load("frameData"));

   b->Return(b->Load("return_retVal"));
   return 1;
   }

int64_t registerPrintString(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *vm = b->Load("vm");
    IlValue *stringID = rb->GetInt64Immediate(b, b->ConstInt64(1));
    IlValue *stringVal =
    b->LoadAt(b->typeDictionary()->PointerTo(b->typeDictionary()->LookupStruct("String")),
    b->      IndexAt(b->typeDictionary()->PointerTo(b->typeDictionary()->PointerTo(b->typeDictionary()->LookupStruct("String"))),
    b->      LoadIndirect("VM", "strings", vm), stringID));
    IlValue *length = b->LoadIndirect("String", "length", stringVal);
    IlValue *data = b->LoadIndirect("String", "data", stringVal);
    b->Call("printStringHelper", 2, data, length);
    rb->DefaultFallthrough(b, b->ConstInt64(9));
    return 0;
}

int64_t registerPrintInt64(RuntimeBuilder *rb, IlBuilder *b) {
    b->Call("printInt64", 1, pop(rb, b));
    rb->DefaultFallthrough(b, b->ConstInt64(1));
    return 0;
}

int64_t registerCurrentTime(RuntimeBuilder *rb, IlBuilder *b) {
    push(rb, b, b->Call("getCurrentTime", 0));
    rb->DefaultFallthrough(b, b->ConstInt64(1));
    return 0;
}

int64_t registerHalt(RuntimeBuilder *rb, IlBuilder *b)
   {
   b->Call("exit", 1, b->ConstInt32(0));
   return 1;
   }
