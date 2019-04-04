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
#include "BytecodeHelpers.hpp"
#include "Helpers.hpp"

#include "IBInterpreter.hpp"
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
using OMR::JitBuilder::VirtualMachineRealArray;
using OMR::JitBuilder::VirtualMachineRealStack;

void IBInterpreter::Setup()
   {
    Store("frame", CreateLocalStruct(typeDictionary()->LookupStruct("Frame")));

    Store("stackDepth", LoadIndirect("Function", "maxStackDepth", Load("function")));
    Store("localCount", LoadIndirect("Function", "localCount", Load("function")));
    Store("dataLength", Add(Load("stackDepth"), Load("localCount")));
    Store("stackSize", Mul(Load("stackDepth"), ConstInt64(sizeof(int64_t))));
    IlBuilder *useStackData = nullptr;
    IlBuilder *useDynamicData = nullptr;
    IfThenElse(&useDynamicData, &useStackData, GreaterThan(Load("dataLength"), ConstInt64(FRAME_INLINED_DATA_LENGTH)));

    useDynamicData->Store("localsSize", useDynamicData->Mul(useDynamicData->Load("localCount"), useDynamicData->ConstInt64(sizeof(int64_t))));
    useDynamicData->Store("frameData", useDynamicData->Call("allocateFrameData", 3, useDynamicData->Load("function"), useDynamicData->Load("stackSize"), useDynamicData->Load("localsSize")));
    useDynamicData->Store("dataPointer", useDynamicData->Load("frameData"));

    useStackData->Store("frameData", useStackData->NullAddress());
    useStackData->Store("dataPointer", useStackData->CreateLocalArray(FRAME_INLINED_DATA_LENGTH, Int64));

    StoreIndirect("Frame", "stack", Load("frame"), Load("dataPointer"));
    StoreIndirect("Frame", "locals", Load("frame"), Add(Load("dataPointer"), Load("stackSize")));
    StoreIndirect("Frame", "args", Load("frame"), Load("a"));
    StoreIndirect("Frame", "previous", Load("frame"), LoadIndirect("VM", "frame", Load("vm")));
    StoreIndirect("VM", "frame", Load("vm"), Load("frame"));

    Store("sp", LoadIndirect("Frame", "stack", Load("frame")));
    Store("args", LoadIndirect("Frame", "args", Load("frame")));
    Store("locals", LoadIndirect("Frame", "locals", Load("frame")));

    VirtualMachineRegisterInStruct *stackRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "stack", "CMInterpreterMethod::SP");
    VirtualMachineRealStack *stack = new VirtualMachineRealStack(this, stackRegister, Int64, true, false);

    VirtualMachineRegisterInStruct *localsRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "locals", "CMInterpreterMethod::LOCALS");
    VirtualMachineRealArray *localsArray = new VirtualMachineRealArray(this, localsRegister, Int64);

    VirtualMachineRegisterInStruct *argsRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "args", "CMInterpreterMethod::ARGS");
    VirtualMachineRealArray *argsArray = new VirtualMachineRealArray(this, argsRegister, Int64);

    InterpreterVMState *vmState = new InterpreterVMState(stack, stackRegister, localsArray, localsRegister, argsArray, argsRegister);
    setVMState(vmState);

    VirtualMachineRegister *opcodes = new VirtualMachineRegisterInStruct(this, "Function", "function", "opcodes", "PC");
    SetPC(opcodes);
   }

IBInterpreter::IBInterpreter(TypeDictionary *types)
   : InterpreterBuilder(types) {
    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);

    IlType *VMType = types->LookupStruct("VM");
    IlType *pVMType = types->PointerTo(VMType);
    IlType *FunctionType = types->LookupStruct("Function");
    IlType *pFunctionType = types->PointerTo(FunctionType);

    DefineName("ib_interpret");
    DefineParameter("vm", pVMType);
    DefineParameter("function", pFunctionType);
    DefineParameter("a", types->pInt64);
    DefineReturnType(Int64);

    DefineLocal("sp", types->pInt64);
    DefineLocal("args", types->pInt64);
    DefineLocal("locals", types->pInt64);

    defineFunctions(this, types);

    registerHandlers(this);
}

void IBInterpreter::registerHandlers(OMR::JitBuilder::RuntimeBuilder *rb) {
    rb->RegisterHandler((int32_t)Bytecodes::NOP, Bytecode::getBytecodeName(Bytecodes::NOP), (void *)&doNop);
    rb->RegisterHandler((int32_t)Bytecodes::PUSH_CONSTANT, Bytecode::getBytecodeName(Bytecodes::PUSH_CONSTANT), (void *)&doPushConstant);
    rb->RegisterHandler((int32_t)Bytecodes::PUSH_ARG, Bytecode::getBytecodeName(Bytecodes::PUSH_ARG), (void *)&doPushArg);
    rb->RegisterHandler((int32_t)Bytecodes::PUSH_LOCAL, Bytecode::getBytecodeName(Bytecodes::PUSH_LOCAL), (void *)&doPushLocal);
    rb->RegisterHandler((int32_t)Bytecodes::POP, Bytecode::getBytecodeName(Bytecodes::POP), (void *)&doPop);
    rb->RegisterHandler((int32_t)Bytecodes::POP_LOCAL, Bytecode::getBytecodeName(Bytecodes::POP_LOCAL), (void *)&doPopLocal);
    rb->RegisterHandler((int32_t)Bytecodes::DUP, Bytecode::getBytecodeName(Bytecodes::DUP), (void *)&doDup);
    rb->RegisterHandler((int32_t)Bytecodes::ADD, Bytecode::getBytecodeName(Bytecodes::ADD), (void *)&doAdd);
    rb->RegisterHandler((int32_t)Bytecodes::SUB, Bytecode::getBytecodeName(Bytecodes::SUB), (void *)&doSub);
    rb->RegisterHandler((int32_t)Bytecodes::MUL, Bytecode::getBytecodeName(Bytecodes::MUL), (void *)&doMul);
    rb->RegisterHandler((int32_t)Bytecodes::DIV, Bytecode::getBytecodeName(Bytecodes::DIV), (void *)&doDiv);
    rb->RegisterHandler((int32_t)Bytecodes::MOD, Bytecode::getBytecodeName(Bytecodes::MOD), (void *)&doMod);
    rb->RegisterHandler((int32_t)Bytecodes::JMP, Bytecode::getBytecodeName(Bytecodes::JMP), (void *)&doJMP);
    rb->RegisterHandler((int32_t)Bytecodes::JMPE, Bytecode::getBytecodeName(Bytecodes::JMPE), (void *)&doJMPE);
    rb->RegisterHandler((int32_t)Bytecodes::JMPL, Bytecode::getBytecodeName(Bytecodes::JMPL), (void *)&doJMPL);
    rb->RegisterHandler((int32_t)Bytecodes::JMPG, Bytecode::getBytecodeName(Bytecodes::JMPG), (void *)&doJMPG);
    rb->RegisterHandler((int32_t)Bytecodes::CALL, Bytecode::getBytecodeName(Bytecodes::CALL), (void *)&doCall);
    rb->RegisterHandler((int32_t)Bytecodes::RET, Bytecode::getBytecodeName(Bytecodes::RET), (void *)&doRet);
    rb->RegisterHandler((int32_t)Bytecodes::PRINT_STRING, Bytecode::getBytecodeName(Bytecodes::PRINT_STRING), (void *)&doPrintString);
    rb->RegisterHandler((int32_t)Bytecodes::PRINT_INT64, Bytecode::getBytecodeName(Bytecodes::PRINT_INT64), (void *)&doPrintInt64);
    rb->RegisterHandler((int32_t)Bytecodes::CURRENT_TIME, Bytecode::getBytecodeName(Bytecodes::CURRENT_TIME), (void *)&doCurrentTime);
    rb->RegisterHandler((int32_t)Bytecodes::HALT, Bytecode::getBytecodeName(Bytecodes::HALT), (void *)&doHalt);
}

void IBInterpreter::defineFunctions(OMR::JitBuilder::RuntimeBuilder *rb, OMR::JitBuilder::TypeDictionary *types) {
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

    rb->DefineFunction((char *)"printString",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&printString,
                  types->NoType,
                  1,
                  types->Int64);

    rb->DefineFunction((char *)"getCurrentTime",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&getCurrentTime,
                  types->Int64,
                  0);

    rb->DefineFunction((char *)"invokeCompiledFunction",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)nullptr,
                  types->Int64,
                  2,
                  pVMType,
                  types->pInt64);

    rb->DefineFunction((char *)"compileFunction",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&compileFunction,
                  types->NoType,
                  2,
                  pVMType,
                  pFunctionType);

    rb->DefineFunction((char *)"allocateFrameData",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&allocateFrameData,
                  types->pInt64,
                  3,
                  pFunctionType,
                  types->Int64,
                  types->Int64);

    rb->DefineFunction((char *)"freeFrameData",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&freeFrameData,
                  types->NoType,
                  1,
                  types->pInt64);

    rb->DefineFunction((char *)"exit",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&exit,
                  types->NoType,
                  1,
                  types->Int32);
}

