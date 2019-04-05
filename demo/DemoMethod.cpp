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
#include "CompiledMethodBuilder.hpp"
#include "RuntimeBuilder.hpp"
#include "DemoMethod.hpp"

using OMR::JitBuilder::IlType;
using OMR::JitBuilder::IlValue;
using OMR::JitBuilder::IlBuilder;
using OMR::JitBuilder::RuntimeBuilder;
using OMR::JitBuilder::TypeDictionary;
using OMR::JitBuilder::VirtualMachineRegister;
using OMR::JitBuilder::VirtualMachineRegisterInStruct;
using OMR::JitBuilder::VirtualMachineOperandStack;
using OMR::JitBuilder::VirtualMachineOperandArray;

DemoMethod::DemoMethod(TypeDictionary *types, VM *vm, Function *function)
   : CompiledMethodBuilder(types, (void *)function->opcodes, 1),
    _function(function) {
    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);

    IlType *VMType = types->LookupStruct("VM");
    IlType *pVMType = types->PointerTo(VMType);
    IlType *pInt64 = types->PointerTo(Int64);

    // Define the API for the interpret function
    DefineName(_function->functionName);
    DefineParameter("vm", pVMType);
    DefineParameter("a", pInt64);
    DefineReturnType(Int64);

    DemoInterpreter::defineFunctions(this, types);

    DefineFunction((char *)"interpret",
                  (char *)__FILE__,
                  (char *)LINETOSTR(__LINE__),
                  (void *)&vm->interpretFunction,
                  Int64,
                  1,
                  pVMType);

    DemoInterpreter::registerHandlers(this);
}

void DemoMethod::Setup() {
    // Create your Frame, stack and locals on the stack
    TypeDictionary *dict = typeDictionary();
    Store("frame", CreateLocalStruct(dict->LookupStruct("Frame")));
    StoreIndirect("Frame", "stack", Load("frame"), CreateLocalArray(_function->maxStackDepth, Int64));
    StoreIndirect("Frame", "locals", Load("frame"), CreateLocalArray(_function->localCount, Int64));
    // Set args to the right parameter
    StoreIndirect("Frame", "args", Load("frame"), Load("a"));

    // Link this frame into the VM... frame->previous = vm->frame; vm->frame = frame;
    StoreIndirect("Frame", "previous", Load("frame"), LoadIndirect("VM", "frame", Load("vm")));
    StoreIndirect("VM", "frame", Load("vm"), Load("frame"));

    // Create a Virtual machine stack to use for stack operations. JIT uses the Operand version instead of Real
    VirtualMachineRegisterInStruct *stackRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "stack", "CMInterpreterMethod::SP");
    VirtualMachineOperandStack *stack = new VirtualMachineOperandStack(this, _function->maxStackDepth, dict->pInt64, stackRegister, true, 0);

    VirtualMachineRegisterInStruct *localsRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "locals", "CMInterpreterMethod::LOCALS");
    VirtualMachineOperandArray *localsArray = new VirtualMachineOperandArray(this, _function->localCount, Int64, localsRegister);

    VirtualMachineRegisterInStruct *argsRegister = new VirtualMachineRegisterInStruct(this, "Frame", "frame", "args", "CMInterpreterMethod::ARGS");
    VirtualMachineOperandArray *argsArray = new VirtualMachineOperandArray(this, _function->argCount, Int64, argsRegister);

    if (_function->argCount > 0) {
        // Need to read the args into the array
        argsArray->Reload(this);
    }

    DemoVMState *vmState = new DemoVMState(stack, stackRegister, localsArray, localsRegister, argsArray, argsRegister);
    setVMState(vmState);
}

