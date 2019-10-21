/*******************************************************************************
 * Copyright (c) 2001, 2017 IBM Corp. and others
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
#include "j9.h"
#include "j9protos.h"
#include "bcnames.h"
#include "j9consts.h"
#include "rommeth.h"
#include "stackwalk.h"
#include "ut_j9vm.h"

UDATA  
dropPendingSendPushes(J9VMThread *currentThread)
{
	UDATA bytecodedFrame = FALSE;
	U_8 * pc = currentThread->pc;

	if ((UDATA) pc <= J9SF_MAX_SPECIAL_FRAME_TYPE) {
			currentThread->sp = (UDATA *) ((UDATA) currentThread->sp + (UDATA) currentThread->literals);
			currentThread->literals = NULL;
			if ((UDATA) pc == J9SF_FRAME_TYPE_JNI_NATIVE_METHOD) {
				J9SFJNINativeMethodFrame * nativeMethodFrame = (J9SFJNINativeMethodFrame *) currentThread->sp;

				nativeMethodFrame->specialFrameFlags &= ~(UDATA)J9_SSF_JNI_PUSHED_REF_COUNT_MASK;
			}
	} else if (*pc == JBimpdep2) {
		/* Call-in frame - drop the pushed arguments, keep any special frame pushes */

		currentThread->sp = (UDATA *) ((UDATA)(currentThread->arg0EA + 1) - sizeof(J9SFJNICallInFrame) - (UDATA)currentThread->literals);
	} else {
		UDATA * bp;

		if (currentThread->literals == NULL) {
			/* invokeExact j2iTransition frame - bp is in arg0EA */

			bp = currentThread->arg0EA;
			Assert_VM_true(bp == currentThread->j2iFrame);
		} else {
			J9Method * method = currentThread->literals;
			J9ROMMethod * romMethod = J9_ROM_METHOD_FROM_RAM_METHOD(method);
			UDATA slotCount = J9_ARG_COUNT_FROM_ROM_METHOD(romMethod) + J9_TEMP_COUNT_FROM_ROM_METHOD(romMethod);


			if (romMethod->modifiers & J9AccSynchronized) {
				++slotCount;
			} else if (J9ROMMETHOD_IS_NON_EMPTY_OBJECT_CONSTRUCTOR(romMethod)) {
				/* Non-empty java.lang.Object.<init> has one hidden temp to hold a copy of the receiver */
				++slotCount;
			}
				
			bp = currentThread->arg0EA - slotCount;
		}

		if (bp == currentThread->j2iFrame) {
			currentThread->sp = (UDATA *) ((UDATA)(bp + 1) - sizeof(J9SFJ2IFrame));				
		} else {
			currentThread->sp = (UDATA *) ((UDATA)(bp + 1) - sizeof(J9SFStackFrame));
		}
		bytecodedFrame = TRUE;
	}

	return bytecodedFrame;
}

void  
prepareForExceptionThrow(J9VMThread *currentThread)
{
	if (dropPendingSendPushes(currentThread)) {
		J9SFSpecialFrame * specialFrame = (J9SFSpecialFrame *) ((U_8 *)currentThread->sp - sizeof(J9SFSpecialFrame));

		specialFrame->specialFrameFlags = 0;
		specialFrame->savedCP = currentThread->literals;
		specialFrame->savedPC = currentThread->pc;
		specialFrame->savedA0 = (UDATA *) ((UDATA)currentThread->arg0EA | J9SF_A0_INVISIBLE_TAG);
		currentThread->arg0EA = currentThread->sp - 1;
		currentThread->sp = (UDATA *) specialFrame;
		currentThread->pc = (U_8 *) J9SF_FRAME_TYPE_GENERIC_SPECIAL;
		currentThread->literals = NULL;
	}
}

void
verifyI2J(J9VMThread *currentThread, const char *reason)
{
	if (currentThread->j2iFrame) {
		PORT_ACCESS_FROM_VMC(currentThread);
		UDATA bad = 0;
		UDATA* returnSP = currentThread->entryLocalStorage->i2jState.returnSP;
		UDATA* a0 = currentThread->entryLocalStorage->i2jState.a0;
		J9Method* literals = currentThread->entryLocalStorage->i2jState.literals;
		UDATA pc = (UDATA)currentThread->entryLocalStorage->i2jState.pc;
		J9JavaStack *stack = currentThread->stackObject;
		UDATA *stackStart = (UDATA*)stack;
		UDATA *stackEnd = stack->end;
		UDATA *untaggedSP = UNTAG2(returnSP, UDATA*);
		J9ClassLoader *cl = NULL;
		J9ROMClass *romClass = zfindROMClassFromPC(currentThread, pc, &cl);

		if (pc == (UDATA)currentThread->javaVM->callInReturnPC) {
			return;
		}
		if ((untaggedSP < stackStart) || (untaggedSP >= stackEnd)) {
			j9tty_printf(PORTLIB, "returnSP outside of stack range\n");
			bad = 1;
		}
		if ((a0 < stackStart) || (a0 >= stackEnd)) {
			j9tty_printf(PORTLIB, "a0 outside of stack range\n");
			bad = 1;
		}
		if(romClass) {
			J9ROMMethod *romMethod = zfindROMMethodInROMClass(currentThread, romClass, pc);
			if (romMethod != NULL) {
				J9ROMMethod *expected = J9_ROM_METHOD_FROM_RAM_METHOD(literals);
				if (romMethod != expected) {
					bad = 1;
					j9tty_printf(PORTLIB, "romMethod for PC (%p) does no match expected (%p)\n", romMethod, expected);
				}
			} else {
				bad = 1;
				j9tty_printf(PORTLIB, "romMethod for PC cannot be located\n");
			}
		} else {
			bad = 1;
			j9tty_printf(PORTLIB, "romClass for PC cannot be located\n");
		}
		if (bad) {
			j9tty_printf(PORTLIB, "<%p> (%s) Bad I2J detected (returnSP %p a0 %p literals %p pc %p) - crashing to produce diagnostics\n", currentThread, reason, returnSP, a0, literals, pc);
			*(UDATA*)-1=-1;
		}
	}
}
