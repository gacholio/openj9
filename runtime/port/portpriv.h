/*******************************************************************************
 * Copyright IBM Corp. and others 1991
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
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#ifndef portlibraryprivatedefines_h
#define portlibraryprivatedefines_h

/* @ddr_namespace: default */
#include "j9cfg.h"
#include "j9comp.h"
#include "omrmutex.h"
#include "omrportptb.h"
#include "j9port.h"
#include "j9portpg.h"
#include <signal.h>

#define FLAG_IS_SET(flag,bitmap)		(0 != (flag & bitmap))
#define FLAG_IS_NOT_SET(flag,bitmap) 	(0 == (flag & bitmap))

/**
 * The "std" file descriptors (fds), stderr,stdout and stdin, have special attributes on z/OS.
 * For example, close() and seek() cannot be called on them.
 * FD_BIAS is used to distinguish the "std" fds on z/OS from other fds.
 * CMVC 99667: Introducing FD-biasing to fix MVS started-tasks and POSIX file-descriptors
 */
#ifdef J9ZOS390
#define FD_BIAS 1000
#else
#define FD_BIAS 0
#endif

/* @ddr_namespace: map_to_type=J9HyperFunctions */

/*
 * Structure that has pointers to hypervisor related functions
 * These get populated with the right set of functions based on which
 * hypervisor we are running on at JVM startup time
 */
typedef struct J9HyperFunctions {
	intptr_t (*get_guest_processor_usage) (struct J9PortLibrary *portLibrary, J9GuestProcessorUsage *gpUsage);
	intptr_t (*get_guest_memory_usage) (struct J9PortLibrary *portLibrary, J9GuestMemoryUsage *gmUsage);
	void (*hypervisor_impl_shutdown) (struct J9PortLibrary *portLibrary);
} J9HyperFunctions;

#define HYPERVISOR_VENDOR_INIT_SUCCESS	0

/* Structure to hold the various attributes of the Hypervisor */
typedef struct J9HypervisorData {
	intptr_t isVirtual;
	J9HypervisorVendorDetails vendorDetails;
	/* Status of hypervisor access from within Guest */
	int32_t vendorStatus;
	/* Detailed error message in case vendor status indicates error */
	char *vendorErrMsg;
	/* Synchronize access for vendor initialization */
	omrthread_monitor_t vendorMonitor;
	/* Hypervisor specific data, will be set at init time */
	void *vendorPrivateData;
	/* These ptrs will be filled in hypervisor init time */
	J9HyperFunctions hypFunc;
} J9HypervisorData;

#define J9SIZEOF_J9HypervisorData sizeof(J9HypervisorData)

#define PHD_isVirtual			(portLibrary->portGlobals->hypervisorData.isVirtual)
#define PHD_vendorDetails		(portLibrary->portGlobals->hypervisorData.vendorDetails)
#define PHD_vendorStatus		(portLibrary->portGlobals->hypervisorData.vendorStatus)
#define PHD_vendorErrMsg		(portLibrary->portGlobals->hypervisorData.vendorErrMsg)
#define PHD_vendorMonitor		(portLibrary->portGlobals->hypervisorData.vendorMonitor)
#define PHD_vendorPrivateData	(portLibrary->portGlobals->hypervisorData.vendorPrivateData)
#define PHD_hypFunc				(portLibrary->portGlobals->hypervisorData.hypFunc)


/**
 * Holds handles and properties relating to a process
 */
typedef struct J9ProcessHandleStruct
{
	intptr_t procHandle;
	intptr_t inHandle;
	intptr_t outHandle;
	intptr_t errHandle;
	int32_t pid;
	intptr_t exitCode;
} J9ProcessHandleStruct;

/* these port library globals are initialized to zero in j9mem_startup_basic */
typedef struct J9PortLibraryGlobalData {
	struct J9PortPlatformGlobals platformGlobals;
	J9HypervisorData hypervisorData;			/* Hypervisor Data */
	omrthread_tls_key_t socketTlsKey;			/* TLS key for j9sock_ptb */
} J9PortLibraryGlobalData;

/* J9SourceJ9GP*/
extern J9_CFUNC void
j9gp_register_handler (struct J9PortLibrary *portLibrary,  handler_fn fn, void *aUserData );
extern J9_CFUNC int32_t
j9gp_startup (struct J9PortLibrary *portLibrary);
extern J9_CFUNC void
j9gp_shutdown (struct J9PortLibrary *portLibrary);
extern J9_CFUNC uintptr_t
j9gp_protect (struct J9PortLibrary *portLibrary,  protected_fn fn, void *arg );
extern J9_CFUNC uint32_t
j9gp_info_count (struct J9PortLibrary *portLibrary, void *info, uint32_t category);
extern J9_CFUNC uint32_t
j9gp_info (struct J9PortLibrary *portLibrary, void *info, uint32_t category, int32_t index, const char **name, void **value);

/* J9SourceJ9IPCMutex*/
extern J9_CFUNC int32_t
j9ipcmutex_release (struct J9PortLibrary *portLibrary, const char *name);
extern J9_CFUNC int32_t
j9ipcmutex_acquire (struct J9PortLibrary *portLibrary, const char *name);
extern J9_CFUNC int32_t
j9ipcmutex_startup (struct J9PortLibrary *portLibrary);
extern J9_CFUNC void
j9ipcmutex_shutdown (struct J9PortLibrary *portLibrary);

/* J9SourceJ9SharedMemory*/
extern J9_CFUNC int32_t
j9shmem_startup (struct J9PortLibrary *portLibrary);
extern J9_CFUNC int32_t
j9shmem_findnext (struct J9PortLibrary *portLibrary, uintptr_t findhandle, char *resultbuf);
extern J9_CFUNC intptr_t
j9shmem_detach (struct J9PortLibrary *portLibrary, struct j9shmem_handle **handle);
extern J9_CFUNC void
j9shmem_findclose (struct J9PortLibrary *portLibrary, uintptr_t findhandle);
extern J9_CFUNC void
j9shmem_close (struct J9PortLibrary *portLibrary, struct j9shmem_handle **handle);
extern J9_CFUNC void*
j9shmem_attach (struct J9PortLibrary *portLibrary, struct j9shmem_handle* handle, uint32_t category);
extern J9_CFUNC uintptr_t
j9shmem_findfirst (struct J9PortLibrary *portLibrary, char *cacheDirName, char *resultbuf);
extern J9_CFUNC void
j9shmem_shutdown (struct J9PortLibrary *portLibrary);
extern J9_CFUNC uintptr_t
j9shmem_stat (struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, const char* name, struct J9PortShmemStatistic* statbuf);
extern J9_CFUNC uintptr_t
j9shmem_statDeprecated (struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, const char* name, struct J9PortShmemStatistic* statbuf, uintptr_t cacheFileType);
extern J9_CFUNC intptr_t
j9shmem_handle_stat(struct J9PortLibrary *portLibrary, struct j9shmem_handle *handle, struct J9PortShmemStatistic *statbuf);
extern J9_CFUNC intptr_t
j9shmem_destroy (struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, struct j9shmem_handle **handle);
extern J9_CFUNC intptr_t
j9shmem_destroyDeprecated (struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, struct j9shmem_handle **handle, uintptr_t cacheFileType);
extern J9_CFUNC intptr_t
j9shmem_open  (struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, struct j9shmem_handle **handle, const char* rootname, uintptr_t size, uint32_t perm, uint32_t category, uintptr_t flags, J9ControlFileStatus *controlFileStatus);
extern J9_CFUNC intptr_t
j9shmem_openDeprecated  (struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, struct j9shmem_handle **handle, const char* rootname, uint32_t perm, uintptr_t cacheFileType, uint32_t category);
extern J9_CFUNC intptr_t
j9shmem_getDir (struct J9PortLibrary *portLibrary, const char* ctrlDirName, uint32_t flags, char* buffer, uintptr_t length);
extern J9_CFUNC intptr_t
j9shmem_createDir (struct J9PortLibrary *portLibrary, char* cacheDirName, uintptr_t cacheDirPerm, BOOLEAN cleanMemorySegments);
extern J9_CFUNC intptr_t
j9shmem_getFilepath (struct J9PortLibrary *portLibrary, char* cacheDirName, char* buffer, uintptr_t length, const char* cachename);
extern J9_CFUNC intptr_t
j9shmem_protect(struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, void *address, uintptr_t length, uintptr_t flags);
extern J9_CFUNC uintptr_t
j9shmem_get_region_granularity(struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, void *address);
extern J9_CFUNC int32_t
j9shmem_getid (struct J9PortLibrary *portLibrary, struct j9shmem_handle* handle);

/* J9SourceJ9SharedSemaphore*/
extern J9_CFUNC int32_t
j9shsem_startup (struct J9PortLibrary *portLibrary);
extern J9_CFUNC void
j9shsem_close  (struct J9PortLibrary *portLibrary, struct j9shsem_handle **handle);
extern J9_CFUNC intptr_t
j9shsem_post (struct J9PortLibrary *portLibrary, struct j9shsem_handle* handle, uintptr_t semset, uintptr_t flag);
extern J9_CFUNC void
j9shsem_shutdown (struct J9PortLibrary *portLibrary);
extern J9_CFUNC int32_t
j9shsem_params_init (struct J9PortLibrary *portLibrary, struct J9PortShSemParameters *params);
extern J9_CFUNC intptr_t
j9shsem_destroy  (struct J9PortLibrary *portLibrary, struct j9shsem_handle **handle);
extern J9_CFUNC intptr_t
j9shsem_setVal (struct J9PortLibrary *portLibrary, struct j9shsem_handle* handle, uintptr_t semset, intptr_t value);
extern J9_CFUNC intptr_t
j9shsem_open  (struct J9PortLibrary *portLibrary, struct j9shsem_handle **handle, const struct J9PortShSemParameters *params);
extern J9_CFUNC intptr_t
j9shsem_getVal (struct J9PortLibrary *portLibrary, struct j9shsem_handle* handle, uintptr_t semset);
extern J9_CFUNC intptr_t
j9shsem_wait (struct J9PortLibrary *portLibrary, struct j9shsem_handle* handle, uintptr_t semset, uintptr_t flag);

/* Deprecated */
extern J9_CFUNC int32_t
j9shsem_deprecated_startup (struct J9PortLibrary *portLibrary);
extern J9_CFUNC void
j9shsem_deprecated_close  (struct J9PortLibrary *portLibrary, struct j9shsem_handle **handle);
extern J9_CFUNC intptr_t
j9shsem_deprecated_post (struct J9PortLibrary *portLibrary, struct j9shsem_handle* handle, uintptr_t semset, uintptr_t flag);
extern J9_CFUNC void
j9shsem_deprecated_shutdown (struct J9PortLibrary *portLibrary);
extern J9_CFUNC intptr_t
j9shsem_deprecated_destroy  (struct J9PortLibrary *portLibrary, struct j9shsem_handle **handle);
extern J9_CFUNC intptr_t
j9shsem_deprecated_destroyDeprecated (struct J9PortLibrary *portLibrary, struct j9shsem_handle **handle, uintptr_t cacheFileType);
extern J9_CFUNC intptr_t
j9shsem_deprecated_setVal (struct J9PortLibrary *portLibrary, struct j9shsem_handle* handle, uintptr_t semset, intptr_t value);
extern J9_CFUNC intptr_t
j9shsem_deprecated_handle_stat(struct J9PortLibrary *portLibrary, struct j9shsem_handle *handle, struct J9PortShsemStatistic *statbuf);
extern J9_CFUNC intptr_t
j9shsem_deprecated_open  (struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, struct j9shsem_handle** handle, const char* semname, int setSize, int permission, uintptr_t flags, J9ControlFileStatus *controlFileStatus);
extern J9_CFUNC intptr_t
j9shsem_deprecated_openDeprecated  (struct J9PortLibrary *portLibrary, const char* cacheDirName, uintptr_t groupPerm, struct j9shsem_handle** handle, const char* semname, uintptr_t cacheFileType);
extern J9_CFUNC int32_t
j9shsem_deprecated_getid (struct J9PortLibrary *portLibrary, struct j9shsem_handle* handle);
extern J9_CFUNC intptr_t
j9shsem_deprecated_getVal (struct J9PortLibrary *portLibrary, struct j9shsem_handle* handle, uintptr_t semset);
extern J9_CFUNC intptr_t
j9shsem_deprecated_wait (struct J9PortLibrary *portLibrary, struct j9shsem_handle* handle, uintptr_t semset, uintptr_t flag);

/* J9SourceJ9SI*/
extern J9_CFUNC int32_t
j9sysinfo_startup (struct J9PortLibrary *portLibrary);
extern J9_CFUNC void
j9sysinfo_shutdown (struct J9PortLibrary *portLibrary);
extern J9_CFUNC uint16_t
j9sysinfo_get_classpathSeparator (struct J9PortLibrary *portLibrary );
extern J9_CFUNC int32_t
j9sysinfo_get_hw_info(struct J9PortLibrary *portLibrary, uint32_t infoType, char * buf, uint32_t bufLen);
extern J9_CFUNC int32_t
j9sysinfo_get_cache_info(struct J9PortLibrary *portLibrary, const J9CacheInfoQuery * query);
extern J9_CFUNC uintptr_t
j9sysinfo_get_processing_capacity (struct J9PortLibrary *portLibrary);
extern J9_CFUNC uintptr_t
j9sysinfo_DLPAR_enabled (struct J9PortLibrary *portLibrary);
extern J9_CFUNC uintptr_t
j9sysinfo_DLPAR_max_CPUs (struct J9PortLibrary *portLibrary);
extern J9_CFUNC uintptr_t
j9sysinfo_weak_memory_consistency (struct J9PortLibrary *portLibrary);

/* J9SourcePort*/
extern J9_CFUNC int32_t
j9port_shutdown_library (struct J9PortLibrary *portLibrary );
extern J9_CFUNC uintptr_t
j9port_getSize (struct J9PortLibraryVersion *version);
extern J9_CFUNC int32_t
j9port_getVersion (struct J9PortLibrary *portLibrary, struct J9PortLibraryVersion *version);
extern J9_CFUNC int32_t
j9port_isCompatible (struct J9PortLibraryVersion *expectedVersion);
extern J9_CFUNC int32_t
j9port_allocate_library (struct J9PortLibraryVersion *version, struct J9PortLibrary **portLibrary);
extern J9_CFUNC int32_t
j9port_isFunctionOverridden (struct J9PortLibrary *portLibrary, uintptr_t offset);
extern J9_CFUNC int32_t
j9port_init_library (struct J9PortLibrary *portLibrary, struct J9PortLibraryVersion *version, uintptr_t size);
extern J9_CFUNC int32_t
j9port_startup_library (struct J9PortLibrary *portLibrary);
extern J9_CFUNC int32_t
j9port_create_library (struct J9PortLibrary *portLibrary, struct J9PortLibraryVersion *version, uintptr_t size);

/* J9SourceSockets*/
extern J9_CFUNC int32_t
j9sock_getaddrinfo (struct J9PortLibrary *portLibrary, char *name, j9addrinfo_t hints, j9addrinfo_t result);
extern J9_CFUNC int32_t
j9sock_shutdown (struct J9PortLibrary *portLibrary );
extern J9_CFUNC int32_t
j9sock_getaddrinfo_create_hints (struct J9PortLibrary *portLibrary, j9addrinfo_t *result, int16_t family, int32_t socktype, int32_t protocol, int32_t flags);
extern J9_CFUNC const char*
j9sock_error_message (struct J9PortLibrary *portLibrary);
extern J9_CFUNC int32_t
j9sock_inetaddr (struct J9PortLibrary *portLibrary, const char *addrStr, uint32_t *addr);
extern J9_CFUNC int32_t
j9sock_gethostbyaddr (struct J9PortLibrary *portLibrary, char *addr, int32_t length, int32_t type, j9hostent_t handle);
extern J9_CFUNC int32_t
j9sock_startup (struct J9PortLibrary *portLibrary);
extern J9_CFUNC int32_t
j9sock_getaddrinfo_name (struct J9PortLibrary *portLibrary, j9addrinfo_t handle, char *name, int index);
extern J9_CFUNC int32_t
j9sock_gethostbyname (struct J9PortLibrary *portLibrary, const char *name, j9hostent_t handle);
extern J9_CFUNC int32_t
j9sock_getaddrinfo_length (struct J9PortLibrary *portLibrary, j9addrinfo_t handle, int32_t *length);
extern J9_CFUNC int32_t
j9sock_freeaddrinfo (struct J9PortLibrary *portLibrary, j9addrinfo_t handle);
extern J9_CFUNC int32_t
j9sock_getaddrinfo_address (struct J9PortLibrary *portLibrary, j9addrinfo_t handle, uint8_t *address, int index, uint32_t* scope_id);
extern J9_CFUNC int32_t
j9sock_getaddrinfo_family (struct J9PortLibrary *portLibrary, j9addrinfo_t handle, int32_t *family, int index );

/* Per-thread buffer for platform-dependent socket information */
struct J9SocketPTB;
extern J9_CFUNC struct J9SocketPTB *j9sock_ptb_get(struct J9PortLibrary *portLibrary);
extern J9_CFUNC int32_t j9sock_ptb_init(struct J9PortLibrary *portLibrary);
extern J9_CFUNC void j9sock_ptb_shutdown(struct J9PortLibrary *portLibrary);

/* J9Process */
extern J9_CFUNC intptr_t
j9process_create (struct J9PortLibrary *portLibrary, const char *command[], uintptr_t commandLength, char *env[], uintptr_t envSize, const char *dir, uint32_t options, intptr_t fdInput, intptr_t fdOutput, intptr_t fdError, J9ProcessHandle *processHandle);
extern J9_CFUNC intptr_t
j9process_waitfor(struct J9PortLibrary *portLibrary, J9ProcessHandle processHandle);
extern J9_CFUNC intptr_t
j9process_terminate (struct J9PortLibrary *portLibrary, J9ProcessHandle processHandle);
extern J9_CFUNC intptr_t
j9process_write (struct J9PortLibrary *portLibrary, J9ProcessHandle processHandle, void *buffer, uintptr_t numBytes);
extern J9_CFUNC intptr_t
j9process_read (struct J9PortLibrary *portLibrary, J9ProcessHandle processHandle, uintptr_t flags, void *buffer, uintptr_t numBytes);
extern J9_CFUNC intptr_t
j9process_get_available (struct J9PortLibrary *portLibrary, J9ProcessHandle processHandle, uintptr_t flags);
extern J9_CFUNC intptr_t
j9process_close (struct J9PortLibrary *portLibrary, J9ProcessHandle *processHandle, uint32_t options);
extern J9_CFUNC intptr_t
j9process_getStream (struct J9PortLibrary *portLibrary, J9ProcessHandle processHandle, uintptr_t streamFlag, intptr_t *stream);
extern J9_CFUNC intptr_t
j9process_isComplete (struct J9PortLibrary *portLibrary, J9ProcessHandle processHandle);
extern J9_CFUNC intptr_t
j9process_get_exitCode (struct J9PortLibrary *portLibrary, J9ProcessHandle processHandle);

#if defined(J9VM_PORT_RUNTIME_INSTRUMENTATION)
/* j9ri */
extern J9_CFUNC void
j9ri_params_init(struct J9PortLibrary *portLibrary, struct J9RIParameters *riParams, void *riControlBlock);
extern J9_CFUNC void
j9ri_initialize(struct J9PortLibrary *portLibrary, struct J9RIParameters *riParams);
extern J9_CFUNC void
j9ri_deinitialize(struct J9PortLibrary *portLibrary, struct J9RIParameters *riParams);
extern J9_CFUNC void
j9ri_enable(struct J9PortLibrary *portLibrary, struct J9RIParameters *riParams);
extern J9_CFUNC void
j9ri_disable(struct J9PortLibrary *portLibrary, struct J9RIParameters *riParams);
extern J9_CFUNC int32_t
j9ri_enableRISupport(struct J9PortLibrary *portLibrary);
extern J9_CFUNC int32_t
j9ri_disableRISupport(struct J9PortLibrary *portLibrary);
#endif /* defined(J9VM_PORT_RUNTIME_INSTRUMENTATION) */

#if defined(OMR_GC_CONCURRENT_SCAVENGER)
extern J9_CFUNC void
j9gs_params_init(struct J9PortLibrary *portLibrary, struct J9GSParameters *gsParams, void *gsControlBlock);
extern J9_CFUNC int32_t
j9gs_initialize(struct J9PortLibrary *portLibrary, struct J9GSParameters *gsParams, int32_t shiftAmount);
extern J9_CFUNC int32_t
j9gs_deinitialize(struct J9PortLibrary *portLibrary, struct J9GSParameters *gsParams);
extern J9_CFUNC void
j9gs_enable(struct J9PortLibrary *portLibrary,  struct J9GSParameters *gsParams, void* baseAddress, uint64_t perBitSectionSize, uint64_t bitMask);
extern J9_CFUNC void
j9gs_disable(struct J9PortLibrary *portLibrary, struct J9GSParameters *gsParams);
extern J9_CFUNC int32_t
j9gs_isEnabled(struct J9PortLibrary *portLibrary, struct J9GSParameters *gsParams, void** baseAddress, uint64_t* perBitSectionSize, uint64_t* bitMask);
#endif /* OMR_GC_CONCURRENT_SCAVENGER */

/* j9hypervisor */
extern J9_CFUNC int32_t
j9hypervisor_startup(struct J9PortLibrary *portLibrary);
extern J9_CFUNC void
j9hypervisor_shutdown(struct J9PortLibrary *portLibrary);
extern J9_CFUNC intptr_t
j9hypervisor_hypervisor_present(struct J9PortLibrary *portLibrary);
extern J9_CFUNC intptr_t
j9hypervisor_get_hypervisor_info(struct J9PortLibrary *portLibrary, J9HypervisorVendorDetails *vendorDetails);
extern J9_CFUNC intptr_t
j9hypervisor_get_guest_processor_usage(struct J9PortLibrary *portLibrary, J9GuestProcessorUsage *gpUsage);
extern J9_CFUNC intptr_t
j9hypervisor_get_guest_memory_usage(struct J9PortLibrary *portLibrary, J9GuestMemoryUsage *gmUsage);

/* j9mem_basic */
extern J9_CFUNC void *
j9mem_allocate_portLibrary(uintptr_t byteAmount);
extern J9_CFUNC void
j9mem_deallocate_portLibrary(void *memoryPointer);

/* J9SourceJ9PortControl*/
extern J9_CFUNC int32_t
j9port_control(struct J9PortLibrary *portLibrary, const char *key, uintptr_t value);

#endif
