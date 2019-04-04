###############################################################################
# Copyright (c) 2017, 2018 IBM Corp. and others
#
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at http://eclipse.org/legal/epl-2.0
# or the Apache License, Version 2.0 which accompanies this distribution
# and is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# This Source Code may also be made available under the following Secondary
# Licenses when the conditions for such availability set forth in the
# Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
# version 2 with the GNU Classpath Exception [1] and GNU General Public
# License, version 2 with the OpenJDK Assembly Exception [2].
#
# [1] https://www.gnu.org/software/classpath/license.html
# [2] http://openjdk.java.net/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
#############################################################################

macro(el_detect_compiler_information)
	if(NOT CMAKE_CXX_COMPILER_ID)
		message(WARNING "EL: CXX Compiler ID is not set. Did you call el_detect_compiler_information before CXX was enabled?")
	else()
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			set(_EL_TOOLCONFIG "gnu")
		elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			set(_EL_TOOLCONFIG "msvc")
		elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$")
			if("x${CMAKE_C_SIMULATE_ID}" STREQUAL "xMSVC" OR "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
				# clang on Windows mimics MSVC
				set(_EL_TOOLCONFIG "msvc")
			else()
				set(_EL_TOOLCONFIG "gnu")
			endif()
		elseif(CMAKE_CXX_COMPILER_ID STREQUAL "XL")
			set(_EL_TOOLCONFIG "xlc")
		else()
			message(FATAL_ERROR "EL: Unknown compiler ID: '${CMAKE_CXX_COMPILER_ID}'")
		endif()
		set(EL_TOOLCONFIG ${_EL_TOOLCONFIG} CACHE STRING "Name of toolchain configuration options to use")
	endif()
endmacro(el_detect_compiler_information)
