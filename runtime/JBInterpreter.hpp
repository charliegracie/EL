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
#include "VirtualMachineRegister.hpp"

#ifndef JB_INTERPRETER_INCL
#define JB_INTERPRETER_INCL

typedef int64_t (JBInterpreterType)(VM *vm, Function *function, int64_t *a);

class JBInterpreter: public OMR::JitBuilder::MethodBuilder {
public:
    JBInterpreter(OMR::JitBuilder::TypeDictionary *types);
    virtual bool buildIL();

private:
    OMR::JitBuilder::IlType *_pInt8;
    OMR::JitBuilder::IlType *_pInt64;
    OMR::JitBuilder::VirtualMachineRegister *_opcodes;

    OMR::JitBuilder::IlValue *getImmediate(OMR::JitBuilder::IlBuilder *builder, int immediate);
    OMR::JitBuilder::IlValue *getArg(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *argIndex);
    void setArg(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *frame, OMR::JitBuilder::IlValue *argIndex, OMR::JitBuilder::IlValue *value);
    OMR::JitBuilder::IlValue *getLocal(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *localIndex);
    void setLocal(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *localIndex, OMR::JitBuilder::IlValue *value);

    void PUSH(OMR::JitBuilder::IlBuilder *builder, OMR::JitBuilder::IlValue *value);
    OMR::JitBuilder::IlValue *POP(OMR::JitBuilder::IlBuilder *builder);

    void initializeCase(Bytecodes bytecode) {
        int32_t bytecodeAsInt = Bytecode::getBytecodeAsInt(bytecode);
        _builders[bytecodeAsInt] = nullptr;
        _cases[bytecodeAsInt] = MakeCase(bytecodeAsInt, &_builders[bytecodeAsInt], false);
    }
    OMR::JitBuilder::IlBuilder *_builders[22];
    OMR::JitBuilder::IlBuilder::JBCase *_cases[22];
};

#endif //JB_INTERPRETER_INCL
