/*******************************************************************************
 * Copyright (c) 2018, 2019 IBM Corp. and others
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

static int reportClassFileLoadHook = 0;
static int watchFieldsOnPrepare = 0;
char const * startingClass = NULL; 
static int watchAllFieldsOnStart = 0;
static int watchFieldsAfterStart = 0;

void
print(jvmtiEnv *jvmti_env, JNIEnv *jni_env, char const *format, ...)
{
	va_list args;
	char newFormat[1024];
	jlong nanos = 0;
	if (NULL != jvmti_env) {
		(*jvmti_env)->GetTime(jvmti_env, &nanos);
	}
	snprintf(newFormat, sizeof(newFormat), "<%p> (%lld) %s", jni_env, nanos, format);
	va_start(args, format);
	vfprintf(stderr, newFormat, args);
	va_end(args);
}

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
	if (reportClassFileLoadHook) {
		print(jvmti_env, jni_env, "classFileLoadHook: %s\n", name ? name : "<NULL>");
	}
}

static void
watchFields(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jclass klass)
{
	char *cname = NULL;
	jint field_count = 0;
	jfieldID *fields = NULL;
	jvmtiError err = (*jvmti_env)->GetClassSignature(jvmti_env, klass, &cname, NULL);
	if (JVMTI_ERROR_NONE != err) {
		cname = NULL;
	}
	print(jvmti_env, jni_env, "\tFields for class %s\n", cname ? cname : "<error from GetClassSignature>");
	err = (*jvmti_env)->GetClassFields(jvmti_env, klass, &field_count, &fields);
	if (JVMTI_ERROR_NONE == err) {
		while (0 != field_count--) {
			jfieldID field = fields[field_count];
			char *name = NULL;
			char *signature = NULL;
			err = (*jvmti_env)->GetFieldName(jvmti_env, klass, field, &name, &signature, NULL);
			if (JVMTI_ERROR_NONE != err) {
				print(jvmti_env, jni_env, "\t\tGetFieldName error %d\n", err);
			} else {
				print(jvmti_env, jni_env, "\t\tfield: %s %s\n", name, signature);
			}
			(*jvmti_env)->Deallocate(jvmti_env, (unsigned char*)signature);
			(*jvmti_env)->Deallocate(jvmti_env, (unsigned char*)name);
			err = (*jvmti_env)->SetFieldAccessWatch(jvmti_env, klass, field);
			if (JVMTI_ERROR_NONE != err) {
				print(jvmti_env, jni_env, "\t\t\tSetFieldAccessWatch error %d\n", err);
			}
			err = (*jvmti_env)->SetFieldModificationWatch(jvmti_env, klass, field);
			if (JVMTI_ERROR_NONE != err) {
				print(jvmti_env, jni_env, "\t\t\tSetFieldModificationWatch error %d\n", err);
			}
		}
	} else {
		print(jvmti_env, jni_env, "\t\tGetClassFields error %d\n", err);
	}
	(*jvmti_env)->Deallocate(jvmti_env, (unsigned char*)fields);
	(*jvmti_env)->Deallocate(jvmti_env, (unsigned char*)cname);
}

void JNICALL
classPrepare(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jclass klass)
{
	int watchAll = 0;
	char *name = NULL;
	jvmtiError err = (*jvmti_env)->GetClassSignature(jvmti_env, klass, &name, NULL);
	if (JVMTI_ERROR_NONE != err) {
		name = NULL;
	}
	print(jvmti_env, jni_env, "classPrepare: %s\n", name ? name : "<error from GetClassSignature>");
	if (watchFieldsOnPrepare) {
		watchFields(jvmti_env, jni_env, klass);
	} else if (NULL != startingClass) {
		if (0 == strcmp(name, startingClass)) {
			print(jvmti_env, jni_env, "\t*** Starting class reached ***\n");
			if (watchFieldsAfterStart) {
				print(jvmti_env, jni_env, "\tStarting field watches for future classes\n");
				watchFieldsOnPrepare = 1;
				if (!watchAllFieldsOnStart) {
					watchFields(jvmti_env, jni_env, klass);
				}
			}
			if (watchAllFieldsOnStart) {
				jint class_count = 0;
				jclass *classes = NULL;
				print(jvmti_env, jni_env, "\tWatching fields for all loaded classes\n");
				err = (*jvmti_env)->GetLoadedClasses(jvmti_env, &class_count, &classes);
				if (JVMTI_ERROR_NONE == err) {
					while (0 != class_count--) {
						watchFields(jvmti_env, jni_env, classes[class_count]);
					}
					(*jvmti_env)->Deallocate(jvmti_env, (unsigned char*)classes);
				} else {
					print(jvmti_env, jni_env, "\tGetLoadedClasses error %d\n", err);
				}
			}
		}
	}
	(*jvmti_env)->Deallocate(jvmti_env, (unsigned char*)name);
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
}

JNIEXPORT jint JNICALL
Agent_Prepare(JavaVM * vm, char *phase, char * options, void * reserved)
{
	jvmtiEnv * jvmti_env = NULL;
	JNIEnv *jni_env = NULL;
	jint rc = JNI_OK;
	jvmtiError err = JVMTI_ERROR_NONE;
	jint version = 0;
	jvmtiEventCallbacks callbacks;
	jvmtiCapabilities capabilities;
	char *token = NULL;

	(*vm)->GetEnv(vm, (void **) &jni_env, JNI_VERSION_1_2);
	print(jvmti_env, jni_env, "%s\n", phase);

	if (options == NULL) {
		options = "";
	}

	token = strtok(options, ",");
	while (NULL != token) {
		if (0 == strcmp("all", token)) {
			watchFieldsOnPrepare = 1;
			print(jvmti_env, jni_env, "Start watching immediately\n");
		}
		if (0 == strcmp("hook", token)) {
			reportClassFileLoadHook = 1;
			print(jvmti_env, jni_env, "Enable ClassFileLoadHook\n");
		}
		if (0 == strncmp("start=", token, 6)) {
			startingClass = token + 6;
			print(jvmti_env, jni_env, "Start watching at class %s\n", startingClass);
		}
		if (0 == strcmp("before", token)) {
			watchAllFieldsOnStart = 0;
			print(jvmti_env, jni_env, "Watch all existing fields once starting class reached\n");
		}
		if (0 == strcmp("after", token)) {
			watchFieldsAfterStart = 1;
			print(jvmti_env, jni_env, "Watch all new fields once starting class reached\n");
		}
		token = strtok(NULL, ",");
	}

	rc = (*vm)->GetEnv(vm, (void **) &jvmti_env, JVMTI_VERSION_1_2);
	if (rc != JNI_OK) {
		if ((rc != JNI_EVERSION) || ((rc = (*vm)->GetEnv(vm, (void **) &jvmti_env, JVMTI_VERSION_1_2)) != JNI_OK)) {
			print(jvmti_env, jni_env, "Failed to GetEnv %d\n", rc);
			return rc;
		}
	}

	/* Capabilities */
	memset(&capabilities, 0, sizeof(jvmtiCapabilities));
	capabilities.can_generate_all_class_hook_events = 1;
	capabilities.can_generate_field_access_events = 1;
	capabilities.can_generate_field_modification_events = 1;
	err = (*jvmti_env)->AddCapabilities(jvmti_env, &capabilities);
	if (err != JVMTI_ERROR_NONE) {
		print(jvmti_env, jni_env, "Failed to add capabilities\n");
		return JNI_ERR;
	}

	/* Callbacks */
	memset(&callbacks, 0, sizeof(jvmtiEventCallbacks));
	callbacks.ClassFileLoadHook = classFileLoadHook;
	callbacks.ClassPrepare = classPrepare;
	callbacks.FieldAccess = fieldAccess;
	callbacks.FieldModification = fieldModification;
	err = (*jvmti_env)->SetEventCallbacks(jvmti_env, &callbacks, sizeof(jvmtiEventCallbacks));
	if (err != JVMTI_ERROR_NONE) {
		print(jvmti_env, jni_env, "Failed to set callbacks\n");
		return JNI_ERR;
	}

	/* Enable the events */
	err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
	if (err != JVMTI_ERROR_NONE) {
		print(jvmti_env, jni_env, "Failed to enable class file load hook event\n");
		return JNI_ERR;
	}
	err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE, NULL);
	if (err != JVMTI_ERROR_NONE) {
		print(jvmti_env, jni_env, "Failed to enable class prepare event\n");
		return JNI_ERR;
	}
	err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_FIELD_ACCESS, NULL);
	if (err != JVMTI_ERROR_NONE) {
		print(jvmti_env, jni_env, "Failed to enable field access event\n");
		return JNI_ERR;
	}
	err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_FIELD_MODIFICATION, NULL);
	if (err != JVMTI_ERROR_NONE) {
		print(jvmti_env, jni_env, "Failed to enable field modification event\n");
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
