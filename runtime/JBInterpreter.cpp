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

#include <time.h>
#include <sys/time.h>

#include "Bytecodes.hpp"
#include "InterpreterTypeDictionary.hpp"
#include "JBInterpreter.hpp"
#include "Helpers.hpp"

using OMR::JitBuilder::IlType;
using OMR::JitBuilder::IlValue;
using OMR::JitBuilder::IlBuilder;
using OMR::JitBuilder::RuntimeBuilder;
using OMR::JitBuilder::TypeDictionary;
using OMR::JitBuilder::VirtualMachineOperandArray;
using OMR::JitBuilder::VirtualMachineOperandStack;
using OMR::JitBuilder::VirtualMachineRegister;
using OMR::JitBuilder::VirtualMachineRegisterInStruct;

#define Instruction(bytecode, bcname) IlBuilder * bcname = _builders[Bytecode::getBytecodeAsInt(bytecode)];

void
handleBadOpcode2(int32_t opcode)
   {
   #define HANDLE_BAD_OPCODE_LINE LINETOSTR(__LINE__)
   fprintf(stderr,"Unknown opcode %d halting the runtime\n", opcode);
   exit(opcode);
   }

JBInterpreter::JBInterpreter(OMR::JitBuilder::TypeDictionary *types) :
        OMR::JitBuilder::MethodBuilder(types),
        _builders(),
        _cases()
{
    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);

    IlType *VMType = types->LookupStruct("VM");
    IlType *pVMType = types->PointerTo(VMType);
    IlType *functionType = types->LookupStruct("Function");
    IlType *pFunctionType = types->PointerTo(functionType);

    _pInt8 = types->PointerTo(Int8);
    _pInt64 = types->PointerTo(Int64);

    DefineFunction((char *)"printString",
                   (char *)__FILE__,
                   (char *)LINETOSTR(__LINE__),
                   (void *)&printString,
                   NoType,
                   1,
                   Int64);

    DefineFunction((char *)"printInt64",
                   (char *)__FILE__,
                   (char *)LINETOSTR(__LINE__),
                   (void *)&printInt64,
                   NoType,
                   1,
                   Int64);

    DefineFunction((char *)"printStringHelper",
                   (char *)__FILE__,
                   (char *)LINETOSTR(__LINE__),
                   (void *)&printStringHelper,
                   NoType,
                   2,
                   Int64,
                   Int64);

    DefineFunction((char *)"getCurrentTime",
                   (char *)__FILE__,
                   (char *)LINETOSTR(__LINE__),
                   (void *)&getCurrentTime,
                   Int64,
                   0);

    DefineFunction((char *)"allocateFrameData",
                   (char *)__FILE__,
                   (char *)LINETOSTR(__LINE__),
                   (void *)&allocateFrameData,
                   _pInt64,
                   3,
                   pFunctionType,
                   Int64,
                   Int64);

    DefineFunction((char *)"freeFrameData",
                   (char *)__FILE__,
                   (char *)LINETOSTR(__LINE__),
                   (void *)&freeFrameData,
                   NoType,
                   1,
                   _pInt64);

    DefineFunction((char *)"InterpreterBuilder::handleBadOpcode",
                   (char *)__FILE__,
                   (char *)HANDLE_BAD_OPCODE_LINE,
                   (void *)&handleBadOpcode2,
                   NoType,
                   1,
                   Int32);

    DefineFunction((char *)"exit",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&exit,
                  types->NoType,
                  1,
                  types->Int32);

    DefineName("jb_interpret");
    DefineParameter("vm", pVMType);
    DefineParameter("function", pFunctionType);
    DefineParameter("a", _pInt64);
    DefineReturnType(Int64);

    DefineLocal("sp", _pInt64);
    DefineLocal("args", _pInt64);
    DefineLocal("locals", _pInt64);

    AllLocalsHaveBeenDefined();
}

IlValue *JBInterpreter::getImmediate(IlBuilder *builder, int immediate) {
    IlValue *offset = builder->ConstInt64(immediate);
    return builder->LoadAt(_pInt64,
           builder->      ConvertTo(_pInt64,
           builder->                Add(
           _opcodes->Load(builder),
           offset)));
}

IlValue *JBInterpreter::getArg(IlBuilder *builder, IlValue *argIndex) {
    return builder->LoadAt(_pInt64,
           builder->       IndexAt(_pInt64,
           builder->               Load("args"), argIndex));
}

void JBInterpreter::setArg(IlBuilder *builder, IlValue *frame, IlValue *argIndex, IlValue *value) {
    IlValue *args = builder->LoadIndirect("Frame", "args", frame);
    builder->StoreAt(
    builder->       IndexAt(_pInt64, args, argIndex), value);
}

IlValue *JBInterpreter::getLocal(IlBuilder *builder, IlValue *localIndex) {
    return builder->LoadAt(_pInt64,
           builder->       IndexAt(_pInt64,
           builder->               Load("locals"), localIndex));
}

void JBInterpreter::setLocal(IlBuilder *builder, IlValue *localIndex, IlValue *value) {
    builder->StoreAt(
    builder->       IndexAt(_pInt64,
    builder->              Load("locals"), localIndex), value);
}

void JBInterpreter::PUSH(IlBuilder *builder, IlValue *value) {
    builder->StoreAt(
    builder->       Load("sp"), value);
    builder->Store("sp",
    builder->     Add(
    builder->         Load("sp"),
    builder->         ConstInt32(8)));
}

IlValue *JBInterpreter::POP(IlBuilder *builder) {
    builder->Store("sp",
    builder->     Sub(
    builder->         Load("sp"),
    builder->         ConstInt32(8)));

    return builder->LoadAt(_pInt64,
           builder->      Load("sp"));
}

#define PEEK(builder) builder->LoadAt(_pInt64, builder->Sub(builder->Load("sp"), builder->ConstInt32(8)))

#define INCREMENT_OPCODES(builder, value) \
do { \
    IlValue *inc = builder->ConstInt64(value); \
    _opcodes->Adjust(builder, inc); \
} while (0)

#define SET_OPCODES(builder, target) \
do { \
    IlValue *newPC = builder->Add(initial, target); \
    _opcodes->Store(builder, newPC); \
} while (0)

#if USE_COMPUTED_GOTO
#define NEXT(builder) \
do { \
    builder->Store("pc", builder->LoadAt(_pInt8, _opcodes->Load(builder))); \
    builder->Store("pcInt", builder->ConvertTo(Int32, builder->Load("pc"))); \
    builder->ComputedGoto("pcInt", &defaultBldr, 22, _cases); \
} while (0)
#else
#define NEXT(builder)
#endif

bool JBInterpreter::buildIL() {
    Store("frame", CreateLocalStruct(typeDictionary()->LookupStruct("Frame")));
    Store("stackDepth", LoadIndirect("Function", "maxStackDepth", Load("function")));
    Store("localCount", LoadIndirect("Function", "localCount", Load("function")));
    Store("dataLength", Add(Load("stackDepth"), Load("localCount")));
    Store("stackSize", Mul(Load("stackDepth"), ConstInt64(sizeof(int64_t))));
    IlBuilder *useStackData = nullptr;
    IlBuilder *useDynamicData = nullptr;
    IfThenElse(&useDynamicData, &useStackData, GreaterThan(Load("dataLength"), ConstInt64(FRAME_INLINED_DATA_LENGTH)));

    useDynamicData->Store("localsSize", useDynamicData->Mul(useDynamicData->Load("localCount"), useDynamicData->ConstInt64(sizeof(int64_t))));
    useDynamicData->Store("data", useDynamicData->Call("allocateFrameData", 3, useDynamicData->Load("function"), useDynamicData->Load("stackSize"), useDynamicData->Load("localsSize")));
    useDynamicData->Store("dataPointer", useDynamicData->Load("data"));

    useStackData->Store("data", useStackData->NullAddress());
    useStackData->Store("dataPointer", useStackData->CreateLocalArray(FRAME_INLINED_DATA_LENGTH, Int64));

    StoreIndirect("Frame", "stack", Load("frame"), Load("dataPointer"));
    StoreIndirect("Frame", "locals", Load("frame"), Add(Load("dataPointer"), Load("stackSize")));
    StoreIndirect("Frame", "args", Load("frame"), Load("a"));
    StoreIndirect("Frame", "previous", Load("frame"), LoadIndirect("VM", "frame", Load("vm")));
    StoreIndirect("VM", "frame", Load("vm"), Load("frame"));

    Store("sp", LoadIndirect("Frame", "stack", Load("frame")));
    Store("args", LoadIndirect("Frame", "args", Load("frame")));
    Store("locals", LoadIndirect("Frame", "locals", Load("frame")));

    _opcodes = new VirtualMachineRegisterInStruct(this, "Function", "function", "opcodes", "PC");
    IlValue *initial = _opcodes->Load(this);

    IlBuilder *defaultBldr = nullptr;
    initializeCase(Bytecodes::NOP);
    initializeCase(Bytecodes::PUSH_CONSTANT);
    initializeCase(Bytecodes::PUSH_ARG);
    initializeCase(Bytecodes::PUSH_LOCAL);
    initializeCase(Bytecodes::POP);
    initializeCase(Bytecodes::POP_LOCAL);
    initializeCase(Bytecodes::DUP);
    initializeCase(Bytecodes::ADD);
    initializeCase(Bytecodes::SUB);
    initializeCase(Bytecodes::MUL);
    initializeCase(Bytecodes::DIV);
    initializeCase(Bytecodes::MOD);
    initializeCase(Bytecodes::JMP);
    initializeCase(Bytecodes::JMPE);
    initializeCase(Bytecodes::JMPL);
    initializeCase(Bytecodes::JMPG);
    initializeCase(Bytecodes::CALL);
    initializeCase(Bytecodes::RET);
    initializeCase(Bytecodes::PRINT_STRING);
    initializeCase(Bytecodes::PRINT_INT64);
    initializeCase(Bytecodes::CURRENT_TIME);
    initializeCase(Bytecodes::HALT);

#if USE_COMPUTED_GOTO
    Store("pc", LoadAt(_pInt8, _opcodes->Load(this)));
    Store("pcInt", ConvertTo(Int32, Load("pc")));

    TableSwitch("pcInt", &defaultBldr, 22, _cases);
#else
    Store("true", ConstInt32(1));
    IlBuilder *loop = NULL;
    WhileDoLoop((char *)"true", &loop);

    loop->Store("pc", loop->LoadAt(_pInt8, _opcodes->Load(loop)));
    loop->Store("pcInt", loop->ConvertTo(Int32, loop->Load("pc")));

    loop->TableSwitch("pcInt", &defaultBldr, 22, _cases);
#endif

    Instruction(Bytecodes::NOP, nop);
    {
        INCREMENT_OPCODES(nop, 1);
        NEXT(nop);
    }

    Instruction(Bytecodes::PUSH_CONSTANT, pushConstant);
    {
        IlValue *constant = getImmediate(pushConstant, IMMEDIATE0);
        PUSH(pushConstant, constant);
        INCREMENT_OPCODES(pushConstant, 9);
        NEXT(pushConstant);
    }

    Instruction(Bytecodes::PUSH_ARG, pushArg)
    {
        PUSH(pushArg, getArg(pushArg, getImmediate(pushArg, IMMEDIATE0)));
        INCREMENT_OPCODES(pushArg, 9);
        NEXT(pushArg);
    }

    Instruction(Bytecodes::PUSH_LOCAL, pushLocal)
    {
        PUSH(pushLocal, getLocal(pushLocal, getImmediate(pushLocal, IMMEDIATE0)));
        INCREMENT_OPCODES(pushLocal, 9);
        NEXT(pushLocal);
    }

    Instruction(Bytecodes::POP, pop)
    {
        POP(pop);
        INCREMENT_OPCODES(pop, 1);
        NEXT(pop);
    }

    Instruction(Bytecodes::POP_LOCAL, popLocal)
    {
        setLocal(popLocal, getImmediate(popLocal, IMMEDIATE0), POP(popLocal));
        INCREMENT_OPCODES(popLocal, 9);
        NEXT(popLocal);
    }

    Instruction(Bytecodes::DUP, dup)
    {
        PUSH(dup, PEEK(dup));
        INCREMENT_OPCODES(dup, 1);
        NEXT(dup);
    }

    Instruction(Bytecodes::ADD, add)
    {
        IlValue *right = POP(add);
        IlValue *left = POP(add);
        PUSH(add, add->Add(left, right));
        INCREMENT_OPCODES(add, 1);
        NEXT(add);
    }

    Instruction(Bytecodes::SUB, sub)
    {
        IlValue *right = POP(sub);
        IlValue *left = POP(sub);
        PUSH(sub, sub->Sub(left, right));
        INCREMENT_OPCODES(sub, 1);
        NEXT(sub);
    }

    Instruction(Bytecodes::MUL, mul)
    {
        IlValue *right = POP(mul);
        IlValue *left = POP(mul);
        PUSH(mul, mul->Mul(left, right));
        INCREMENT_OPCODES(mul, 1);
        NEXT(mul);
    }

    Instruction(Bytecodes::DIV, div)
    {
        IlValue *right = POP(div);
        IlValue *left = POP(div);
        PUSH(div, div->Div(left, right));
        INCREMENT_OPCODES(div, 1);
        NEXT(div);
    }

    Instruction(Bytecodes::MOD, mod);
    {
        IlValue *right = POP(mod);
        IlValue *left = POP(mod);
        PUSH(mod, mod->Rem(left, right));
        INCREMENT_OPCODES(mod, 1);
        NEXT(mod);
    }

    Instruction(Bytecodes::JMP, jmp)
    {
        IlValue *target = getImmediate(jmp, IMMEDIATE0);
        SET_OPCODES(jmp, target);
        NEXT(jmp);
    }

    Instruction(Bytecodes::JMPE, jmpe)
    {
        IlValue *right = POP(jmpe);
        IlValue *left = POP(jmpe);
        IlBuilder *ifBldr = NULL;
        IlBuilder *elseBldr = NULL;
        IlValue *target = getImmediate(jmpe, IMMEDIATE0);
        jmpe->IfThenElse(&ifBldr, &elseBldr,
        jmpe->          EqualTo(left, right));
        SET_OPCODES(ifBldr, target);
        INCREMENT_OPCODES(elseBldr, 9);
        NEXT(jmpe);
    }

    Instruction(Bytecodes::JMPL, jmpl)
    {
        IlValue *right = POP(jmpl);
        IlValue *left = POP(jmpl);
        IlBuilder *ifBldr = NULL;
        IlBuilder *elseBldr = NULL;
        IlValue *target = getImmediate(jmpl, IMMEDIATE0);
        jmpl->IfThenElse(&ifBldr, &elseBldr,
        jmpl->          LessThan(left, right));
        SET_OPCODES(ifBldr, target);
        INCREMENT_OPCODES(elseBldr, 9);
        NEXT(jmpl);
    }

    Instruction(Bytecodes::JMPG, jmpg)
    {
        IlValue *right = POP(jmpg);
        IlValue *left = POP(jmpg);
        IlBuilder *ifBldr = NULL;
        IlBuilder *elseBldr = NULL;
        IlValue *target = getImmediate(jmpg, IMMEDIATE0);
        jmpg->IfThenElse(&ifBldr, &elseBldr,
        jmpg->          GreaterThan(left, right));
        SET_OPCODES(ifBldr, target);
        INCREMENT_OPCODES(elseBldr, 9);
        NEXT(jmpg);
    }

    Instruction(Bytecodes::CALL, call)
    {
        IlValue *functionID = getImmediate(call, IMMEDIATE0);
        IlValue *argCount = getImmediate(call, IMMEDIATE1);
        call->Store("newFunction",
        call->     LoadAt(typeDictionary()->PointerTo(typeDictionary()->LookupStruct("Function")),
        call->     IndexAt(typeDictionary()->PointerTo(typeDictionary()->PointerTo(typeDictionary()->LookupStruct("Function"))),
        call->            LoadIndirect("VM", "functions",
        call->                        Load("vm")), functionID)));

        call->Store("argCountSize",
        call->        Mul(
        call->           ConstInt64(sizeof(int64_t)),
                         argCount));

        call->Store("newArgs",
        call->     Sub(
        call->        Load("sp"),
        call->        Load("argCountSize")));

        call->StoreIndirect("Frame", "stack",
        call->             Load("frame"),
        call->             Load("sp"));

        call->Store("call_retVal",
        call->     Call("jb_interpret", 3,
        call->         Load("vm"),
        call->         Load("newFunction"),
        call->         Load("newArgs")));

        call->Store("sp",
        call->     Load("newArgs"));

        PUSH(call, call->Load("call_retVal"));
        INCREMENT_OPCODES(call, 17);
        NEXT(call);
    }

    Instruction(Bytecodes::RET, ret)
    {
        ret->Store("return_retVal", POP(ret));
        ret->StoreIndirect("VM", "frame",
        ret->             Load("vm"),
        ret->             LoadIndirect("Frame", "previous",
        ret->                         Load("frame")));
        IlBuilder *freeData = nullptr;
        ret->IfThen(&freeData, ret->NotEqualTo(ret->Load("data"), ret->NullAddress()));
        freeData->Call("freeFrameData", 1, freeData->Load("data"));
        ret->Return(ret->Load("return_retVal"));
    }

    Instruction(Bytecodes::PRINT_STRING, printString)
    {
        IlValue *stringID = getImmediate(printString, IMMEDIATE0);
        IlValue *stringVal =
        printString->LoadAt(typeDictionary()->PointerTo(typeDictionary()->LookupStruct("String")),
        printString->      IndexAt(typeDictionary()->PointerTo(typeDictionary()->PointerTo(typeDictionary()->LookupStruct("String"))),
        printString->      LoadIndirect("VM", "strings", printString->Load("vm")), stringID));
        IlValue *length = printString->LoadIndirect("String", "length", stringVal);
        IlValue *data = printString->LoadIndirect("String", "data", stringVal);
        printString->Call("printStringHelper", 2, data, length);
        INCREMENT_OPCODES(printString, 9);
        NEXT(printString);
    }

    Instruction(Bytecodes::PRINT_INT64, printInt64)
    {
        printInt64->Call("printInt64", 1, POP(printInt64));
        INCREMENT_OPCODES(printInt64, 1);
        NEXT(printInt64);
    }

    Instruction(Bytecodes::CURRENT_TIME, currentTime)
    {
        PUSH(currentTime, currentTime->Call("getCurrentTime", 0));
        INCREMENT_OPCODES(currentTime, 1);
        NEXT(currentTime);
    }

    Instruction(Bytecodes::HALT, halt)
    {
        halt->Call("exit", 1, halt->ConstInt32(0));
    }

#if USE_COMPUTED_GOTO == 0
    defaultBldr->Call("InterpreterBuilder::handleBadOpcode", 1, defaultBldr->ConvertTo(Int32, defaultBldr->Load("pc")));
#endif

    Return(ConstInt64(-1));

    return true;
}

