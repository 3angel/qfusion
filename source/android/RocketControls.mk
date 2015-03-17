LOCAL_PATH := $(QFUSION_PATH)/libsrcs/libRocket/libRocket
include $(CLEAR_VARS)
LOCAL_MODULE := RocketControls

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_C_INCLUDES := $(LOCAL_EXPORT_C_INCLUDES)

LOCAL_SRC_FILES := $(addprefix Source/Controls/,$(notdir $(wildcard $(LOCAL_PATH)/Source/Controls/*.cpp)))

include $(BUILD_STATIC_LIBRARY)
