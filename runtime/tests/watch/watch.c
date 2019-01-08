/*******************************************************************************
 * Copyright (c) 2018, 2018 IBM Corp. and others
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
#include <string.h>
#include "jvmti.h"

static int reportHook = 0;

void JNICALL
classFileLoadHook(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jclass class_being_redefined,
            jobject loader,
            const char* name,
            jobject protection_domain,
            jint class_data_len,
            const unsigned char* class_data,
            jint* new_class_data_len,
            unsigned char** new_class_data)
{
	if (reportHook) {
		printf("classFileLoadHook: %s\n", name ? name : "<NULL>");
	}
}

void JNICALL
classLoad(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jclass klass)
{
}

void JNICALL
fieldAccess(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method,
            jlocation location,
            jclass field_klass,
            jobject object,
            jfieldID field)
{
}

void JNICALL
fieldModification(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method,
            jlocation location,
            jclass field_klass,
            jobject object,
            jfieldID field,
            char signature_type,
            jvalue new_value)
{
}

JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM * vm)
{
#if 0
	if (env != NULL) {
		tprintf(env, 100, "Agent_OnUnload\n");
		freeArguments(env);
	}
#endif
}

JNIEXPORT jint JNICALL
Agent_Prepare(JavaVM * vm, char *phase, char * options, void * reserved)
{
	jvmtiEnv * jvmti_env;
	jint rc;
	jvmtiError err;
	jint version;
	jvmtiEventCallbacks callbacks;
	jvmtiCapabilities capabilities;

	if (options == NULL) {
		options = "";
	}

	rc = (*vm)->GetEnv(vm, (void **) &jvmti_env, JVMTI_VERSION_1_2);
	if (rc != JNI_OK) {
		if ((rc != JNI_EVERSION) || ((rc = (*vm)->GetEnv(vm, (void **) &jvmti_env, JVMTI_VERSION_1_2)) != JNI_OK)) {
			error(env, err, "Failed to GetEnv %d\n", rc);
			goto done;
		}
	}

	/* Capabilities */
	memset(&capabilities, 0, sizeof(jvmtiCapabilities));
	capabilities.can_generate_all_class_hook_events = 1;
	capabilities.can_generate_field_access_events = 1;
	capabilities.can_generate_field_modification_events = 1;
	err = (*jvmti_env)->AddCapabilities(jvmti_env, &capabilities);
	if (err != JVMTI_ERROR_NONE) {
		error(agent_env, err, "Failed to add capabilities");
		return JNI_ERR;
	}

	/* Callbacks */
	memset(&callbacks, 0, sizeof(jvmtiEventCallbacks));
	callbacks.ClassFileLoadHook = classFileLoadHook;
	callbacks.ClassLoad = classLoad;
	callbacks.FieldAccess = fieldAccess;
	callbacks.FieldModification = fieldModification;
	err = (*jvmti_env)->SetEventCallbacks(jvmti_env, &callbacks, sizeof(jvmtiEventCallbacks));
	if (err != JVMTI_ERROR_NONE) {
		error(agent_env, err, "Failed to set callbacks");
		return JNI_ERR;
	}

	/* Enable the events */
	err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
	if (err != JVMTI_ERROR_NONE) {
		error(agent_env, err, "Failed to enable class file load hook event");
		return JNI_ERR;
	}
	err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_CLASS_LOAD, NULL);
	if (err != JVMTI_ERROR_NONE) {
		error(agent_env, err, "Failed to enable class load event");
		return JNI_ERR;
	}
	err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_FIELD_ACCESS, NULL);
	if (err != JVMTI_ERROR_NONE) {
		error(agent_env, err, "Failed to enable field access event");
		return JNI_ERR;
	}
	err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_FIELD_MODIFICATION, NULL);
	if (err != JVMTI_ERROR_NONE) {
		error(agent_env, err, "Failed to enable field modification event");
		return JNI_ERR;
	}

	return rc;
}

JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM * vm, char * options, void * reserved)
{
	return Agent_Prepare(vm, "Agent_OnLoad", options, reserved);
}

JNIEXPORT jint JNICALL
Agent_OnAttach(JavaVM * vm, char * options, void * reserved)
{
	return Agent_Prepare(vm, "Agent_OnAttach", options, reserved);
}
