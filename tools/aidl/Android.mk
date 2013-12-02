# Copyright 2007 The Android Open Source Project
#
# Copies files into the directory structure described by a manifest

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	aidl_language_l.l \
	aidl_language_y.y \
	aidl.cpp \
	aidl_language.cpp \
	options.cpp \
	search_path.cpp \
	AST.cpp \
	Type.cpp \
	generate_java.cpp

LOCAL_CFLAGS := -g
LOCAL_MODULE := aidl

include $(BUILD_HOST_EXECUTABLE)


