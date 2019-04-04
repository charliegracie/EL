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

#include <time.h>
#include <sys/time.h>

#include "EL.hpp"
#include "Bytecodes.hpp"
#include "BytecodeHelpers.hpp"
#include "Helpers.hpp"
#include "InterpreterTypeDictionary.hpp"

#include "CMInterpreterMethod.hpp"
#include "IlBuilder.hpp"
#include "CompiledMethodBuilder.hpp"
#include "RuntimeBuilder.hpp"
#include "IBInterpreter.hpp"

using OMR::JitBuilder::BytecodeBuilder;
using OMR::JitBuilder::IlType;
using OMR::JitBuilder::IlValue;
using OMR::JitBuilder::IlBuilder;
using OMR::JitBuilder::RuntimeBuilder;
using OMR::JitBuilder::TypeDictionary;
using OMR::JitBuilder::VirtualMachineOperandArray;
using OMR::JitBuilder::VirtualMachineOperandStack;
using OMR::JitBuilder::VirtualMachineRegister;
using OMR::JitBuilder::VirtualMachineRegisterInStruct;

void CMInterpreterMethod::Setup() {
    TypeDictionary *types = typeDictionary();
    Store("frame", CreateLocalStruct(types->LookupStruct("Frame")));
    Store("compiled_stack", CreateLocalArray(_function->maxStackDepth, Int64));
    Store("compiled_locals", CreateLocalArray(_function->localCount, Int64));

    Store("frameData", NullAddress());

    StoreIndirect("Frame", "stack", Load("frame"), Load("compiled_stack"));
    StoreIndirect("Frame", "locals", Load("frame"), Load("compiled_locals"));
    StoreIndirect("Frame", "args", Load("frame"), Load("a"));

    StoreIndirect("Frame", "previous", Load("frame"), LoadIndirect("VM", "frame", Load("vm")));
    StoreIndirect("VM", "frame", Load("vm"), Load("frame"));

    Store("sp", LoadIndirect("Frame", "stack", Load("frame")));
    Store("args", LoadIndirect("Frame", "args", Load("frame")));
    Store("locals", LoadIndirect("Frame", "locals", Load("frame")));

    VirtualMachineRegisterInStruct *stackRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "stack", "CMInterpreterMethod::SP");
    VirtualMachineOperandStack *stack = new VirtualMachineOperandStack(this, _function->maxStackDepth, types->pInt64, stackRegister, true, 0);

    VirtualMachineRegisterInStruct *localsRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "locals", "CMInterpreterMethod::LOCALS");
    VirtualMachineOperandArray *localsArray = new VirtualMachineOperandArray(this, _function->localCount, Int64, localsRegister);

    VirtualMachineRegisterInStruct *argsRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "args", "CMInterpreterMethod::ARGS");
    VirtualMachineOperandArray *argsArray = new VirtualMachineOperandArray(this, _function->argCount, Int64, argsRegister);

    if (_function->argCount > 0) {
        // Need to read the args into the array
        argsArray->Reload(this);
    }

    InterpreterVMState *vmState = new InterpreterVMState(stack, stackRegister, localsArray, localsRegister, argsArray, argsRegister);
    setVMState(vmState);
}

CMInterpreterMethod::CMInterpreterMethod(TypeDictionary *types, VM *vm, Function *func)
    : CompiledMethodBuilder(types, (void *)func->opcodes, 1),
    _function(func)
{
    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);

    IlType *VMType = types->LookupStruct("VM");
    IlType *pVMType = types->PointerTo(VMType);

    DefineName(func->functionName);

    DefineParameter("vm", pVMType);
    DefineParameter("a", types->pInt64);
    DefineReturnType(Int64);

    DefineLocal("sp", types->pInt64);
    DefineLocal("args", types->pInt64);
    DefineLocal("locals", types->pInt64);

    DefineFunction((char *)"ib_interpret",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&vm->interpretFunction,
                  Int64,
                  1,
                  pVMType);

    IBInterpreter::defineFunctions(this, types);
    IBInterpreter::registerHandlers(this);
}
