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

#include "EL.hpp"
#include "JitBuilder.hpp"

#ifndef INTERPRETER_TYPES_INCL
#define INTERPRETER_TYPES_INCL

class InterpreterTypeDictionary: public OMR::JitBuilder::TypeDictionary {
public:
    InterpreterTypeDictionary() :
        OMR::JitBuilder::TypeDictionary() {
        DefineStruct("Function");
        DefineField("Function", "functionName", PointerTo(Int8), offsetof(Function, functionName));
        DefineField("Function", "functionID", Int64, offsetof(Function, functionID));
        DefineField("Function", "invokedCount", Int64, offsetof(Function, invokedCount));
        DefineField("Function", "compiledFunction", Address, offsetof(Function, compiledFunction));
        DefineField("Function", "maxStackDepth", Int64, offsetof(Function, maxStackDepth));
        DefineField("Function", "argCount", Int64, offsetof(Function, argCount));
        DefineField("Function", "localCount", Int64, offsetof(Function, localCount));
        DefineField("Function", "opcodeCount", Int64, offsetof(Function, opcodeCount));
        DefineField("Function", "opcodes", PointerTo(Int8), offsetof(Function, opcodes));
        CloseStruct("Function");

        DefineStruct("String");
        DefineField("String", "length", Int64, offsetof(String, length));
        DefineField("String", "data", PointerTo(Int8), offsetof(String, data));
        CloseStruct("String");

        DefineStruct("Frame");
        DefineField("Frame", "previous", PointerTo(LookupStruct("Frame")), offsetof(Frame, previous));
        DefineField("Frame", "function", PointerTo(LookupStruct("Function")), offsetof(Frame, function));
        DefineField("Frame", "stack", PointerTo(Int64), offsetof(Frame, stack));
        DefineField("Frame", "args", PointerTo(Int64), offsetof(Frame, args));
        DefineField("Frame", "locals", PointerTo(Int64), offsetof(Frame, locals));
        CloseStruct("Frame");

        DefineStruct("VM");
        DefineField("VM", "functions", PointerTo(PointerTo(LookupStruct("Function"))), offsetof(VM, functions));
        DefineField("VM", "strings", PointerTo(PointerTo(LookupStruct("String"))), offsetof(VM, strings));
        DefineField("VM", "frame", PointerTo(LookupStruct("Frame")), offsetof(VM, frame));
        CloseStruct("VM");
    }
};

#endif /* INTERPRETER_TYPES_INCL */
