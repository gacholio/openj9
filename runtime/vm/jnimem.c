/*******************************************************************************
 * Copyright (c) 1991, 2021 IBM Corp. and others
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
#include "j9port.h"
#include "ut_j9vm.h"
#include "vm_internal.h"


void* jniArrayAllocateMemoryFromThread(J9VMThread* vmThread, UDATA sizeInBytes)
{
#ifdef J9VM_GC_JNI_ARRAY_CACHE
	J9JNICache **prev = &vmThread->jniArrayCache;
	J9JNICache *cache = *prev;
	UDATA actualSize = sizeInBytes + sizeof(J9JNICache);
	while (NULL != cache) {
		if (cache->size >= actualSize) {
			Trc_VM_jniArrayCache_hit(vmThread, actualSize);
			*prev = cache->next;
			vmThread->jniArrayCacheCount -= 1;
			break;
		}
		prev = &cache->next;
		cache = cache->next;
	}
	if (NULL == cache) {
		PORT_ACCESS_FROM_VMC(vmThread);
		if (NULL == vmThread->jniArrayCache) {
			Trc_VM_jniArrayCache_missUsed(vmThread, actualSize);
		} else {
			Trc_VM_jniArrayCache_missBigger(vmThread, actualSize);
		}
		cache = j9mem_allocate_memory(actualSize, J9MEM_CATEGORY_JNI);
		if (NULL == cache) {
			return NULL;
		}
		cache->size = actualSize;
		cache->next = NULL;
	}
	return cache + 1;
#else
	PORT_ACCESS_FROM_VMC(vmThread);
	return j9mem_allocate_memory(sizeInBytes, J9MEM_CATEGORY_JNI);
#endif
}

void* jniArrayAllocateMemory32FromThread(J9VMThread* vmThread, UDATA sizeInBytes)
{
	PORT_ACCESS_FROM_VMC(vmThread);
	return j9mem_allocate_memory32(sizeInBytes, J9MEM_CATEGORY_JNI);
}

void jniArrayFreeMemoryFromThread(J9VMThread* vmThread, void* location)
{
	PORT_ACCESS_FROM_VMC(vmThread);

#ifdef J9VM_GC_JNI_ARRAY_CACHE
	J9JNICache *block = ((J9JNICache*)location) - 1;
	UDATA blockSize = block->size;
	J9JavaVM* vm = vmThread->javaVM;

	location = block;

	/* If the block is too large, it never gets cached */
	if (blockSize <= vm->jniArrayCacheMaxSize) {
		/* If the maximum count has not yet been reached, cache this block */
		if (vmThread->jniArrayCacheCount < vm->jniArrayCacheMaxCount) {
			block->next = vmThread->jniArrayCache;
			vmThread->jniArrayCache = block;
			vmThread->jniArrayCacheCount += 1;
			location = NULL;
		} else {
			/* See if there is a smaller cache which can be replaced with this one */
			J9JNICache **prev = &vmThread->jniArrayCache;
			J9JNICache *cache = *prev;
			while (NULL != cache) {
				if (cache->size < blockSize) {
					location = cache;
					block->next = cache->next;
					*prev = block;
					break;
				}
				prev = &cache->next;
				cache = cache->next;
			}
		}		
	}
#endif
	j9mem_free_memory(location);
}

void jniArrayFreeMemory32FromThread(J9VMThread* vmThread, void* location)
{
	PORT_ACCESS_FROM_VMC(vmThread);
	j9mem_free_memory32(location);
}

#if (defined(J9VM_GC_JNI_ARRAY_CACHE)) 
void cleanupVMThreadJniArrayCache(J9VMThread *vmThread)
{
	PORT_ACCESS_FROM_VMC(vmThread);
	J9JNICache *cache = vmThread->jniArrayCache;
	while (NULL != cache) {
		J9JNICache *next = cache->next;
		j9mem_free_memory(cache);
		cache = next;
	}
	vmThread->jniArrayCache = NULL;
	vmThread->jniArrayCacheCount = 0;
}
#endif /* J9VM_GC_JNI_ARRAY_CACHE */



