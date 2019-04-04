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

#include "EL.hpp"

void printString(int64_t ptr) {
#define PRINTSTRING_LINE LINETOSTR(__LINE__)
    char *string = (char *) ptr;
    printf("%s", string);
}

void printStringHelper(int64_t ptr, int64_t length) {
#define PRINTSTRINGHELPER_LINE LINETOSTR(__LINE__)
    char *string = (char *) ptr;
    fprintf(stdout, "%.*s", (int32_t)length, string);
}

void printInt64(int64_t val) {
#define PRINTINT64_LINE LINETOSTR(__LINE__)
    fprintf(stdout, "%lld", val);
}
int64_t getCurrentTime(int64_t val) {
#define GETCURRENTTIME_LINE LINETOSTR(__LINE__)
    struct timeval tp;
    gettimeofday(&tp, NULL);
    int64_t time = ((int64_t)tp.tv_sec) * 1000 + tp.tv_usec / 1000;
    return time;
}

int64_t *allocateFrameData(Function *function, int64_t stackSize, int64_t localsSize) {
    int64_t *data = (int64_t *)malloc(stackSize + localsSize);
    if (nullptr == data) {
        fprintf(stderr, "Error creating stack and locals for function %s....exiting\n", function->functionName);
        exit(-1);
    }
    return data;
}

void freeFrameData(int64_t *data) {
    free(data);
}
