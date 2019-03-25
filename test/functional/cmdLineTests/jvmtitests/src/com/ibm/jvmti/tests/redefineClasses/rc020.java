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
package com.ibm.jvmti.tests.redefineClasses;

import java.io.*;

import com.ibm.jvmti.tests.util.Util;

import priv.*;

public class rc020  {
	public static native boolean redefineClass(Class name, int classBytesSize,
			byte[] classBytes);

 public static byte[] getClassBytes(String name) throws Throwable {
                FileInputStream in = new FileInputStream("/team/gac/redef/priv/"+name+".new");
                ByteArrayOutputStream out = new ByteArrayOutputStream();
                int b;
                while ((b = in.read()) != -1) {
                        out.write(b);
                }
                in.close();
                return out.toByteArray();
        }

	public boolean testRedefineObject() throws Throwable {
		System.out.println("calling to resolve");
		PrivSub o = new PrivSub();
		try { o.m(); } catch(Throwable t) { t.printStackTrace(); }
		System.out.println("redefining");
		byte b[] = getClassBytes("PrivMid");
		redefineClass(PrivMid.class, b.length, b);
		System.out.println("calling after redefine");
		try { o.m(); } catch(Throwable t) { t.printStackTrace(); }
		return true;

	}

	public String helpRedefineObject() {
		return "Tests redefining java.lang.Object";
	}
}
