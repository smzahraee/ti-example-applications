LOCAL_PATH:= $(call my-dir)

####### statcoll  ########################################

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= statcoll.c \
		  Dra7xx_ddrstat_speed.c

LOCAL_MODULE := statcoll
LOCAL_STATIC_LIBRARIES:= libc libcutils
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(BUILD_EXECUTABLE)

###########################################################
