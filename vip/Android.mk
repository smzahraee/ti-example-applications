LOCAL_PATH:= $(call my-dir)

####### sensor-cfg ########################################

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= sensor-cfg.c
LOCAL_MODULE := sensor-cfg
LOCAL_STATIC_LIBRARIES:= libc libcutils
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(BUILD_EXECUTABLE)

###########################################################
