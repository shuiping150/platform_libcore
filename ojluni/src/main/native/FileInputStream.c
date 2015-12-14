/*
 * Copyright (c) 1997, 2007, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "jni.h"
#include "jni_util.h"
#include "jlong.h"
#include "io_util.h"
#include "io_util_md.h"

#include "jvm.h"

#include "java_io_FileInputStream.h"

#include <fcntl.h>
#include <limits.h>

#include "io_util_md.h"
#include "JNIHelp.h"

#define NATIVE_METHOD(className, functionName, signature) \
{ #functionName, signature, (void*)(className ## _ ## functionName) }

/*******************************************************************/
/*  BEGIN JNI ********* BEGIN JNI *********** BEGIN JNI ************/
/*******************************************************************/

jfieldID fis_fd; /* id for jobject 'fd' in java.io.FileInputStream */

/**************************************************************
 * static methods to store field ID's in initializers
 */

JNIEXPORT void JNICALL
FileInputStream_initIDs(JNIEnv *env, jclass fdClass) {
    fis_fd = (*env)->GetFieldID(env, fdClass, "fd", "Ljava/io/FileDescriptor;");
}

/**************************************************************
 * Input stream
 */

JNIEXPORT void JNICALL
FileInputStream_open(JNIEnv *env, jobject this, jstring path) {
    fileOpen(env, this, path, fis_fd, O_RDONLY);
}

JNIEXPORT jint JNICALL
FileInputStream_read0(JNIEnv *env, jobject this) {
    return readSingle(env, this, fis_fd);
}

JNIEXPORT jint JNICALL
FileInputStream_readBytes(JNIEnv *env, jobject this,
        jbyteArray bytes, jint off, jint len) {
    return readBytes(env, this, bytes, off, len, fis_fd);
}

JNIEXPORT jlong JNICALL
FileInputStream_skip0(JNIEnv *env, jobject this, jlong toSkip) {
    jlong cur = jlong_zero;
    jlong end = jlong_zero;
    FD fd = GET_FD(this, fis_fd);
    if (fd == -1) {
        JNU_ThrowIOException (env, "Stream Closed");
        return 0;
    }
    if ((cur = IO_Lseek(fd, (jlong)0, (jint)SEEK_CUR)) == -1) {
      if (errno == ESPIPE) {
        JNU_ThrowByName(env, "java/io/FileInputStream$UseManualSkipException", NULL);
      } else {
        JNU_ThrowIOExceptionWithLastError(env, "Seek error");
      }
    } else if ((end = IO_Lseek(fd, toSkip, (jint)SEEK_CUR)) == -1) {
        JNU_ThrowIOExceptionWithLastError(env, "Seek error");
    }
    return (end - cur);
}

static int available(int fd, jlong *bytes) {
  int n;
  // Unlike the original OpenJdk implementation, we use FIONREAD for all file
  // types. For regular files, this is specified to return the difference
  // between the current position and the file size. Note that this can be
  // negative if we're positioned past the end of the file. We must return 0
  // in that case.
  if (ioctl(fd, FIONREAD, &n) != -1) {
    if (n < 0) {
      n = 0;
    }
    *bytes = n;
    return 1;
  }

  // FIONREAD is specified to return ENOTTY when fd refers to a file
  // type for which this ioctl isn't implemented.
  if (errno == ENOTTY) {
    *bytes = 0;
    return 1;
  }

  // Raise an exception for all other error types.
  return 0;
}

JNIEXPORT jint JNICALL
FileInputStream_available(JNIEnv *env, jobject this) {
    jlong ret;
    FD fd = GET_FD(this, fis_fd);
    if (fd == -1) {
        JNU_ThrowIOException (env, "Stream Closed");
        return 0;
    }
    if (available(fd, &ret)) {
        if (ret > INT_MAX) {
            ret = (jlong) INT_MAX;
        }
        return jlong_to_jint(ret);
    }
    JNU_ThrowIOExceptionWithLastError(env, NULL);
    return 0;
}

JNIEXPORT void JNICALL
FileInputStream_close0(JNIEnv *env, jobject this) {
    fileClose(env, this, fis_fd);
}

static JNINativeMethod gMethods[] = {
  NATIVE_METHOD(FileInputStream, initIDs, "()V"),
  NATIVE_METHOD(FileInputStream, open, "(Ljava/lang/String;)V"),
  NATIVE_METHOD(FileInputStream, skip0, "(J)J"),
  NATIVE_METHOD(FileInputStream, available, "()I"),
  //NATIVE_METHOD(FileInputStream, read0, "()I"),
  //NATIVE_METHOD(FileInputStream, readBytes, "([BII)I"),
  //NATIVE_METHOD(FileInputStream, close0, "()V"),
};

void register_java_io_FileInputStream(JNIEnv* env) {
  jniRegisterNativeMethods(env, "java/io/FileInputStream", gMethods, NELEM(gMethods));
}