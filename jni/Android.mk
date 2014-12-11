TOP_LOCAL_PATH := $(call my-dir)
include $(call all-subdir-makefiles)
LOCAL_PATH := $(TOP_LOCAL_PATH)

include $(CLEAR_VARS)
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := com_akkessler_pspider_pspider.c
LOCAL_MODULE    := com_akkessler_pspider_pspider
include $(BUILD_SHARED_LIBRARY)