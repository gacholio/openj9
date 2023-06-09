/*******************************************************************************
 * Copyright IBM Corp. and others 2001
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
package com.ibm.jvmti.tests.redefineClasses;

import java.lang.reflect.Method;
import java.net.URL;

import com.ibm.jvmti.tests.util.Util;

public class rc014 {
	public static native boolean redefineClass(Class name, int classBytesSize, byte[] classBytes);
	public static native boolean redefineClassExpectFailure(Class name, int classBytesSize, byte[] classBytes);

	public boolean setup(String args) {
		return true;
	}

	private static boolean methodExists(Class klass, String methodName, Class[] parameterTypes) {
		try {
			Method method = klass.getMethod(methodName, parameterTypes);
			if (method != null) {
				return true;
			}
		} catch (Exception e) {
		}
		return false;
	}

	// rather than calling !methodExists(), this specifically checks for a NoSuchMethodException
	private static boolean noSuchMethod(Class klass, String methodName, Class[] parameterTypes) {
		try {
			klass.getMethod(methodName, parameterTypes);
		} catch (NoSuchMethodException e) {
			return true;
		} catch (Exception e) {
		}
		return false;
	}

	//****************************************************************************************
	// 
	// 

	public boolean testAddFinalizer() {
		rc014_testAddFinalizer_O1 t_pre = new rc014_testAddFinalizer_O1();

		// original version has a meth1 but no meth2
		if (!methodExists(t_pre.getClass(), "meth1", null)) {
			return false;
		}

		boolean redefined = Util.redefineClassExpectFailure(getClass(), rc014_testAddFinalizer_O1.class, rc014_testAddFinalizer_R1.class);
		if (redefined) {
			System.out.println("class was redefined when we expected it not to be");
			/* we expected this to fail */
			return false;
		}

		return true;
	}

	public String helpAddFinalizer() {
		return "Attempt to add a finalizer.  This should fail"; 
	}

	//****************************************************************************************
	// 
	// 

	public boolean testDeleteFinalizer() {
		rc014_testDeleteFinalizer_O1 t_pre = new rc014_testDeleteFinalizer_O1();

		// original version has a meth1 and a finalizer
		if (!methodExists(t_pre.getClass(), "meth1", null)) {
			return false;
		} 
		
		boolean redefined = Util.redefineClassExpectFailure(getClass(), rc014_testDeleteFinalizer_O1.class, rc014_testDeleteFinalizer_R1.class);
		if (redefined) {
			/* we expected it to fail the redefinition */
			return false;
		}

		return true;
	}
	
	public String helpDeleteFinalizer()
	{
		return "Attempt to delete a finalizer.  This should fail"; 
	}
	
	//****************************************************************************************
	// 
	// we handle finalizers which are forwarders specially.  Without HCR we don't have the field that 
	// would allow us to support changing the method to something that actually does something.  
	// We need to ensure that for HCR the field is added and modifying it works

	public boolean testModifyFinalizerForwarder() {
		rc014_testModifyFinalizerForwarder_O1 t_pre = new rc014_testModifyFinalizerForwarder_O1();

		boolean redefined = Util.redefineClass(getClass(), rc014_testModifyFinalizerForwarder_O1.class, rc014_testModifyFinalizerForwarder_R1.class);
		if (!redefined) {
			return false;
		}

		// now transform back
		redefined = Util.redefineClass(getClass(), rc014_testModifyFinalizerForwarder_R1.class, rc014_testModifyFinalizerForwarder_O1.class);
		if (!redefined) {
			return false;
		}
		return true;
	}
	
	public String helpModifyFinalizerForwarder()
	{
		return "Modify a forwarder finalizer to a regular finalizer and back"; 
	}
	
	//****************************************************************************************
	// 
	// we handle finalizers which are empty specially.  Without HCR we don't have the field that 
	// would allow us to support changing the method to something that actually does something.  
	// We need to ensure that for HCR the field is added and modifying it works

	public boolean testModifyFinalizerEmpty() {
		rc014_testModifyFinalizerEmpty_O1 t_pre = new rc014_testModifyFinalizerEmpty_O1();

		boolean redefined = Util.redefineClass(getClass(), rc014_testModifyFinalizerEmpty_O1.class, rc014_testModifyFinalizerEmpty_R1.class);
		if (!redefined) {
			return false;
		}

		// now transform back
		redefined = Util.redefineClass(getClass(), rc014_testModifyFinalizerEmpty_R1.class, rc014_testModifyFinalizerEmpty_O1.class);
		if (!redefined) {
			return false;
		}
		return true;
	}
	
	public String helpModifyFinalizerEmpty()
	{
		return "Modify an empty finalizer to a regular finalizer and back"; 
	}
	
	
	//****************************************************************************************
	// 
	// Validate that we don't allow the finalize method in java.lang.Object to be transformed to
	// be non-empty.  Note this test is too dependent on the Object shape to be run normally but is included here
	// commented out to make any future manual testing easier.  In order to run this test in its current form you also have
	// to remove the check in classIsModifiable in hshelp which prevents java.lang.Object from being replaced and also comment out the check that the
	// superclasses are the same in verifyClassesAreCompatible
	public boolean testModifyJavaLangObjectFinalizer() {
		try {
			
			// get the bytes for the current Object implementation
			// in order to update the test create a new version of java.lang.Object and point this line so it reads that new
			// version and uncomment the lines which output the class bytes, once you have the output with the updated
			// bytes update the newClassBytesString and then point it back to the original
			byte classBytes[] = Util.getClassBytesFromUrl(Object.class,new URL("file:/" + System.getProperty("java.home") + "/jre/lib/amd64/compressedrefs/jclSC180/vm.jar"));

			boolean redefined = Util.redefineClass(getClass(), Object.class, classBytes);
			if (!redefined) {
				return false;
			}
	
			return true;
		} catch (Exception e){
			System.out.println("Unexpected exception:" + e);
			return false;
		}
	}
	
	public String helpModifyJavaLangObjectFinalizer()
	{
		return "Validate that we don't allow the finalize method in java.lang.Object to be transformed"; 
	}

}
