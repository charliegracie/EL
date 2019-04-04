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
#include <stdarg.h>
#include <cstring>
#include <cstddef>
#include <iostream>
#include <fstream>

#include "EL.hpp"

#ifndef ELPARSER_INCL
#define ELPARSER_INCL

#if defined(EL_IS_BIG_ENDIAN)
#define convertToPlatformEndian(x)
#else
#if defined(__linux__)
#include <byteswap.h>
#define convertToPlatformEndian(x) x = bswap_64(x)
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define convertToPlatformEndian(x) x = OSSwapInt64(x)
#elif defined(_MSC_VER)
#include <stdlib.h>
#define convertToPlatformEndian(x) x = _byteswap_uint64(x)
#else
#error Platform does not provide convertToPlatformEndian functionality
#endif
#endif

class ELParser {
public:
    ELParser(const char *fileName);
    ~ELParser();

    bool initialize();
    Program *parseProgram();

private:
    const char *_fileName;
    std::ifstream _infile;
    Program _program;

    int parseEyecatcher();
    int parseNameLength();
    char * parseName(int nameLength);
    int parseFunctionCount();
    Function * parseFunction();
    int64_t parseFunctionSize();
    int64_t parseInt64();
    int8_t * parseFunctionOpcodes(int64_t opcodeCount, int64_t *functionMaxStackSize, int64_t *localCount);
    int64_t read64(int8_t *bytes);
    void write64(int8_t *opcodes, int64_t val);
};

#endif /*ELPARSER_INCL */
