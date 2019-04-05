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


#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "Bytecodes.hpp"
#include "InterpreterTypeDictionary.hpp"
#include "DemoHelpers.hpp"

#include "DemoInterpreter.hpp"
#include "IlBuilder.hpp"
#include "InterpreterBuilder.hpp"
#include "RuntimeBuilder.hpp"

using OMR::JitBuilder::IlType;
using OMR::JitBuilder::IlValue;
using OMR::JitBuilder::IlBuilder;
using OMR::JitBuilder::RuntimeBuilder;
using OMR::JitBuilder::TypeDictionary;
using OMR::JitBuilder::VirtualMachineRegister;
using OMR::JitBuilder::VirtualMachineRegisterInStruct;
using OMR::JitBuilder::VirtualMachineRealStack;
using OMR::JitBuilder::VirtualMachineRealArray;

DemoInterpreter::DemoInterpreter(TypeDictionary *types)
   : InterpreterBuilder(types) {
    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);

    IlType *VMType = types->LookupStruct("VM");
    IlType *pVMType = types->PointerTo(VMType);
    IlType *FunctionType = types->LookupStruct("Function");
    IlType *pFunctionType = types->PointerTo(FunctionType);
    IlType *pInt64 = types->PointerTo(Int64);

    // Define the API for the interpret function
    DefineName("interpret");
    DefineParameter("vm", pVMType);
    DefineParameter("function", pFunctionType);
    DefineParameter("a", pInt64);
    DefineReturnType(Int64);

    defineFunctions(this, types);

    registerHandlers(this);
}

void DemoInterpreter::Setup() {
    // Create your Frame on the stack
    Store("frame", CreateLocalStruct(typeDictionary()->LookupStruct("Frame")));

    // Check the stack requirements
    Store("stackDepth", LoadIndirect("Function", "maxStackDepth", Load("function")));
    IlBuilder *stackTooBigExit = nullptr;
    IfThen(&stackTooBigExit, GreaterThan(Load("stackDepth"), ConstInt64(FRAME_INLINED_DATA_LENGTH)));
    // If the requirements are too big exit
    stackTooBigExit->Call("exit", 1, stackTooBigExit->ConstInt32(-1));

    Store("localCount", LoadIndirect("Function", "localCount", Load("function")));
    IlBuilder *localTooBigExit = nullptr;
    IfThen(&localTooBigExit, GreaterThan(Load("localCount"), ConstInt64(FRAME_INLINED_DATA_LENGTH)));
    // If the requirements are too big exit
    localTooBigExit->Call("exit", 1, localTooBigExit->ConstInt32(-1));

    StoreIndirect("Frame", "stack", Load("frame"), CreateLocalArray(FRAME_INLINED_DATA_LENGTH, Int64));
    StoreIndirect("Frame", "locals", Load("frame"), CreateLocalArray(FRAME_INLINED_DATA_LENGTH, Int64));
    StoreIndirect("Frame", "args", Load("frame"), Load("a"));

    // Link this frame into the VM... frame->previous = vm->frame; vm->frame = frame;
    StoreIndirect("Frame", "previous", Load("frame"), LoadIndirect("VM", "frame", Load("vm")));
    StoreIndirect("VM", "frame", Load("vm"), Load("frame"));

    // Create a Virtual machine stack to use for stack operations <--- more important in the JIT'd
    VirtualMachineRegisterInStruct *stackRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "stack", "SP");
    VirtualMachineRealStack *stack = new VirtualMachineRealStack(this, stackRegister, Int64, true, false);

    VirtualMachineRegisterInStruct *localsRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "locals", "LOCALS");
    VirtualMachineRealArray *localsArray = new VirtualMachineRealArray(this, localsRegister, Int64);

    VirtualMachineRegisterInStruct *argsRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "args", "ARGS");
    VirtualMachineRealArray *argsArray = new VirtualMachineRealArray(this, argsRegister, Int64);

    DemoVMState *vmState = new DemoVMState(stack, stackRegister, localsArray, localsRegister, argsArray, argsRegister);
    setVMState(vmState);

    // Create a VM register for the bytecode stream
    VirtualMachineRegister *opcodes = new VirtualMachineRegisterInStruct(this, "Function", "function", "opcodes", "PC");
    SetPC(opcodes);
}

void DemoInterpreter::registerHandlers(OMR::JitBuilder::RuntimeBuilder *rb) {
    //Register bytecode handlers
    rb->RegisterHandler((int32_t)Bytecodes::NOP, Bytecode::getBytecodeName(Bytecodes::NOP), (void *)&DemoInterpreter::doNop);
    rb->RegisterHandler((int32_t)Bytecodes::HALT, Bytecode::getBytecodeName(Bytecodes::HALT), (void *)&DemoInterpreter::doHalt);

    rb->RegisterHandler((int32_t)Bytecodes::PUSH_CONSTANT, Bytecode::getBytecodeName(Bytecodes::PUSH_CONSTANT), (void *)&DemoInterpreter::doPushConstant);
    rb->RegisterHandler((int32_t)Bytecodes::ADD, Bytecode::getBytecodeName(Bytecodes::ADD), (void *)&DemoInterpreter::doAdd);
    rb->RegisterHandler((int32_t)Bytecodes::RET, Bytecode::getBytecodeName(Bytecodes::RET), (void *)&DemoInterpreter::doReturn);

    rb->RegisterHandler((int32_t)Bytecodes::PRINT_STRING, Bytecode::getBytecodeName(Bytecodes::PRINT_STRING), (void *)&DemoInterpreter::doPrintString);
    rb->RegisterHandler((int32_t)Bytecodes::PRINT_INT64, Bytecode::getBytecodeName(Bytecodes::PRINT_INT64), (void *)&DemoInterpreter::doPrintInt64);

    rb->RegisterHandler((int32_t)Bytecodes::JMP, Bytecode::getBytecodeName(Bytecodes::JMP), (void *)&DemoInterpreter::doJMP);
    rb->RegisterHandler((int32_t)Bytecodes::JMPL, Bytecode::getBytecodeName(Bytecodes::JMPL), (void *)&DemoInterpreter::doJMPL);

    //rb->RegisterHandler((int32_t)Bytecodes::CALL, Bytecode::getBytecodeName(Bytecodes::CALL), (void *)&DemoInterpreter::doCall);

    rb->RegisterHandler((int32_t)Bytecodes::PUSH_LOCAL, Bytecode::getBytecodeName(Bytecodes::PUSH_LOCAL), (void *)&DemoInterpreter::doPushLocal);
    rb->RegisterHandler((int32_t)Bytecodes::POP_LOCAL, Bytecode::getBytecodeName(Bytecodes::POP_LOCAL), (void *)&DemoInterpreter::doPopLocal);
    rb->RegisterHandler((int32_t)Bytecodes::MUL, Bytecode::getBytecodeName(Bytecodes::MUL), (void *)&DemoInterpreter::doMul);

    rb->RegisterHandler((int32_t)Bytecodes::CALL, Bytecode::getBytecodeName(Bytecodes::CALL), (void *)&DemoInterpreter::doCallWithJIT);

    rb->RegisterHandler((int32_t)Bytecodes::CURRENT_TIME, Bytecode::getBytecodeName(Bytecodes::CURRENT_TIME), (void *)&DemoInterpreter::doCurrentTime);
    rb->RegisterHandler((int32_t)Bytecodes::PUSH_ARG, Bytecode::getBytecodeName(Bytecodes::PUSH_ARG), (void *)&DemoInterpreter::doPushArg);
    rb->RegisterHandler((int32_t)Bytecodes::DUP, Bytecode::getBytecodeName(Bytecodes::DUP), (void *)&DemoInterpreter::doDup);
    rb->RegisterHandler((int32_t)Bytecodes::SUB, Bytecode::getBytecodeName(Bytecodes::SUB), (void *)&DemoInterpreter::doSub);
    rb->RegisterHandler((int32_t)Bytecodes::POP, Bytecode::getBytecodeName(Bytecodes::POP), (void *)&DemoInterpreter::doPop);
}

//opcodes += NOP_LENGTH;
int64_t DemoInterpreter::doNop(RuntimeBuilder *rb, IlBuilder *b) {
    rb->DefaultFallthrough(b, b->ConstInt64(NOP_LENGTH));
    return 0;
}

//exit(0);
int64_t DemoInterpreter::doHalt(RuntimeBuilder *rb, IlBuilder *b) {
    b->Call("exit", 1, b->ConstInt32(0));
    return 1;
}

//int64_t constant = getImmediate(opcodes, IMMEDIATE0);
//PUSH(constant);
//opcodes += PUSH_CONSTANT_LENGTH;
int64_t DemoInterpreter::doPushConstant(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *constant = rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE0));
    state->_stack->Push(b, constant);

    rb->DefaultFallthrough(b, b->ConstInt64(PUSH_CONSTANT_LENGTH));
    return 0;
}

//int64_t right = POP();
//int64_t left = POP();
//PUSH(left + right);
//opcodes += ADD_LENGTH;
int64_t DemoInterpreter::doAdd(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *right = state->_stack->Pop(b);
    IlValue *left = state->_stack->Pop(b);
    IlValue *result = b->Add(left, right);
    state->_stack->Push(b, result);

    rb->DefaultFallthrough(b, b->ConstInt64(ADD_LENGTH));
    return 0;
}

//int64_t retVal = POP();
//vm->frame = frame->previous;
//return retVal;
int64_t DemoInterpreter::doReturn(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *retVal = state->_stack->Pop(b);

    b->StoreIndirect("VM", "frame",
    b->             Load("vm"),
    b->             LoadIndirect("Frame", "previous",
    b->                         Load("frame")));

    b->Return(retVal);
    return 1;
}

// int64_t immediate = getImmediate();
// String str = vm->strings[immediate]
// printStringHelper(str.length, str.data)
int64_t DemoInterpreter::doPrintString(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *vm = b->Load("vm");

    IlValue *stringID = rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE0));

    IlValue *stringVal =
    b->LoadAt(b->typeDictionary()->PointerTo(b->typeDictionary()->LookupStruct("String")),
    b->      IndexAt(b->typeDictionary()->PointerTo(b->typeDictionary()->PointerTo(b->typeDictionary()->LookupStruct("String"))),
    b->      LoadIndirect("VM", "strings", vm), stringID));

    IlValue *length = b->LoadIndirect("String", "length", stringVal);
    IlValue *data = b->LoadIndirect("String", "data", stringVal);

    b->Call("printStringHelper", 2, data, length);

    rb->DefaultFallthrough(b, b->ConstInt64(PRINT_STRING_LENGTH));
    return 0;
}

//int64_t val = POP();
//printInt64(val);
//opcodes += PRINT_INT64_LENGTH;
int64_t DemoInterpreter::doPrintInt64(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *val = state->_stack->Pop(b);
    b->Call("printInt64", 1, val);

    rb->DefaultFallthrough(b, b->ConstInt64(PRINT_INT64_LENGTH));
    return 0;
}

//opcodes = target;
int64_t DemoInterpreter::doJMP(RuntimeBuilder *rb, IlBuilder *b) {
    IlValue *target = rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE0));

    rb->Jump(b, target, true);
    return 0;
}

//right = POP();
//left = POP();
//if (left < right) {
//   opcodes = target;
//} else {
//   opcodes += JMPL_LENGTH;
//}
int64_t DemoInterpreter::doJMPL(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *right = state->_stack->Pop(b);
    IlValue *left = state->_stack->Pop(b);
    IlValue *cond = b->LessThan(left, right);
    IlValue *target = rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE0));

    rb->JumpIfOrFallthrough(b, cond, target, b->ConstInt64(JMPL_LENGTH), true);
    return 0;
}

// functionID = getImmediate(IMMEDIATE0);
// argCount = getImmediate(IMMEDIATE1)
// newFunction = vm->functions[functionID);
// newArgs = frame->stack - argCount
// result = interpret(vm, newFunction, newArgs)
// push(result)
int64_t DemoInterpreter::doCall(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    TypeDictionary *dict = b->typeDictionary();
    IlType *ppFunctionType = dict->PointerTo(dict->PointerTo(dict->LookupStruct("Function")));
    IlValue *functionID = rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE0));
    IlValue *argCount = rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE1));

    b->Store("newFunction",
    b->     LoadAt(ppFunctionType,
    b->     IndexAt(ppFunctionType,
    b->            LoadIndirect("VM", "functions",
    b->                        Load("vm")), functionID)));

    b->Store("argCountSize",
    b->        Mul(
    b->           ConstInt64(sizeof(int64_t)),
                  argCount));

    state->Commit(b);
    b->Store("newArgs",
    b->     Sub(
               state->_stackTop->Load(b),
    b->        Load("argCountSize")));

    b->Store("call_retVal",
    b->     Call("interpret", 3,
    b->         Load("vm"),
    b->         Load("newFunction"),
    b->         Load("newArgs")));

    state->_stack->Drop(b, rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE1)));
    state->_stack->Push(b, b->Load("call_retVal"));

    rb->DefaultFallthrough(b, b->ConstInt64(CALL_LENGTH));

    return 0;
}

//localIndex = getImmediate(opcodes, IMMEDIATE0)
//local = frame->locals[localIndex];
//PUSH(local);
//opcodes += 9;
int64_t DemoInterpreter::doPushLocal(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *localIndex = rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE0));
    IlValue *local = state->_locals->Get(b, localIndex);
    state->_stack->Push(b, local);

    rb->DefaultFallthrough(b, b->ConstInt64(PUSH_LOCAL_LENGTH));
    return 0;
}

//local = POP()
//localIndex = getImmediate(opcodes, IMMEDIATE0)
//frame->locals[localIndex] = local
int64_t DemoInterpreter::doPopLocal(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *localIndex = rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE0));
    IlValue *local = state->_stack->Pop(b);
    state->_locals->Set(b, localIndex, b->Copy(local));

    rb->DefaultFallthrough(b, b->ConstInt64(POP_LOCAL_LENGTH));
    return 0;
}

//int64_t right = POP();
//int64_t left = POP();
//PUSH(left * right);
//opcodes += MUL_LENGTH;
int64_t DemoInterpreter::doMul(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *right = state->_stack->Pop(b);
    IlValue *left = state->_stack->Pop(b);
    IlValue *result = b->Mul(left, right);
    state->_stack->Push(b, result);

    rb->DefaultFallthrough(b, b->ConstInt64(MUL_LENGTH));
    return 0;
}

// functionID = getImmediate(IMMEDIATE0);
// argCount = getImmediate(IMMEDIATE1)
// newFunction = vm->functions[functionID);
// newArgs = frame->stack - argCount
// int64_t invokedCount = newFunction->invokedCount;
// if (invokedCount < INVOCATIONS_BEFORE_COMPILE) {
//    newFunction->invokedCount += 1;
//    if (invokedCount == INVOCATIONS_BEFORE_COMPILE) {
//        compileFunction(vm, newFunction);
//    }
// }
// void *compiledFunction = newFunction->compiledFunction;
// if (nullptr != compiledFunction) {
//    result = compiledFunction(vm, newFunction, args);
// } else {
//    result = interpret(vm, newFunction, args);
// }
// push(result)
int64_t DemoInterpreter::doCallWithJIT(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));
    IlValue *functionID = rb->GetInt64Immediate(b, b->ConstInt64(1));
    IlValue *argCount = rb->GetInt64Immediate(b, b->ConstInt64(9));

    b->Store("newFunction",
    b->     LoadAt(b->typeDictionary()->PointerTo(b->typeDictionary()->LookupStruct("Function")),
    b->           IndexAt(b->typeDictionary()->PointerTo(b->typeDictionary()->PointerTo(b->typeDictionary()->LookupStruct("Function"))),
    b->                   LoadIndirect("VM", "functions", b->Load("vm")), functionID)));

    b->Store("argCountSize",
    b->        Mul(
    b->           ConstInt64(sizeof(int64_t)),
                  argCount));

    state->Commit(b);
    b->Store("newArgs",
    b->     Sub(
               state->_stackTop->Load(b),
    b->        Load("argCountSize")));

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

    compileBuilder->Call("compile", 2,
    compileBuilder->    Load("vm"),
    compileBuilder->    Load("newFunction"));

    b->Store("compiledFunction",
    b->     LoadIndirect("Function", "compiledFunction",
    b->                 Load("newFunction")));

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

    callInterpreter->Store("call_retVal",
    callInterpreter->     Call("interpret", 3, callInterpreter->Load("vm"), callInterpreter->Load("newFunction"), callInterpreter->Load("newArgs")));

    state->_stack->Drop(b, rb->GetInt64Immediate(b, b->ConstInt64(9)));

    state->_stack->Push(b, b->Load("call_retVal"));

    rb->DefaultFallthrough(b, b->ConstInt64(CALL_LENGTH));

    return 0;
}

int64_t DemoInterpreter::doCurrentTime(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *currentTime = b->Call("getCurrentTime", 0);
    state->_stack->Push(b, currentTime);

    rb->DefaultFallthrough(b, b->ConstInt64(CURRENT_TIME_LENGTH));
    return 0;
}

//argIndex = getImmediate(opcodes, IMMEDIATE0)
//arg = frame->args[argIndex];
//PUSH(arg);
//opcodes += 9;
int64_t DemoInterpreter::doPushArg(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *argIndex = rb->GetInt64Immediate(b, b->ConstInt64(IMMEDIATE0));
    IlValue *arg = state->_args->Get(b, argIndex);
    state->_stack->Push(b, arg);

    rb->DefaultFallthrough(b, b->ConstInt64(PUSH_ARG_LENGTH));
    return 0;
}

int64_t DemoInterpreter::doDup(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    state->_stack->Dup(b);

    rb->DefaultFallthrough(b, b->ConstInt64(DUP_LENGTH));
    return 0;
}

//int64_t right = POP();
//int64_t left = POP();
//PUSH(left - right);
//opcodes += SUB_LENGTH;
int64_t DemoInterpreter::doSub(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    IlValue *right = state->_stack->Pop(b);
    IlValue *left = state->_stack->Pop(b);
    IlValue *result = b->Sub(left, right);
    state->_stack->Push(b, result);

    rb->DefaultFallthrough(b, b->ConstInt64(SUB_LENGTH));
    return 0;
}

int64_t DemoInterpreter::doPop(RuntimeBuilder *rb, IlBuilder *b) {
    DemoVMState *state = static_cast<DemoVMState *>(rb->GetVMState(b));

    state->_stack->Pop(b);

    rb->DefaultFallthrough(b, b->ConstInt64(DUP_LENGTH));
    return 0;
}

void DemoInterpreter::defineFunctions(OMR::JitBuilder::RuntimeBuilder *rb, TypeDictionary *types) {
    IlType *VMType = types->LookupStruct("VM");
    IlType *pVMType = types->PointerTo(VMType);
    IlType *FunctionType = types->LookupStruct("Function");
    IlType *pFunctionType = types->PointerTo(FunctionType);

    rb->DefineFunction((char *)"printStringHelper",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&printStringHelper,
                  types->NoType,
                  2,
                  types->Int64,
                  types->Int64);

    rb->DefineFunction((char *)"printInt64",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&printInt64,
                  types->NoType,
                  1,
                  types->Int64);

    rb->DefineFunction((char *)"getCurrentTime",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&getCurrentTime,
                  types->Int64,
                  0);

    rb->DefineFunction((char *)"exit",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&exit,
                  types->NoType,
                  1,
                  types->Int32);

    rb->DefineFunction((char *)"invokeCompiledFunction",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)nullptr,
                  types->Int64,
                  2,
                  pVMType,
                  types->pInt64);

    rb->DefineFunction((char *)"compile",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&compile,
                  types->NoType,
                  2,
                  pVMType,
                  pFunctionType);
}
