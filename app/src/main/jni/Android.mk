LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_LDLIBS := -llog

LOCAL_MODULE:= libtest

LOCAL_SRC_FILES := com_gwcd_sy_clib_LibTest.c

include $(BUILD_SHARED_LIBRARY)