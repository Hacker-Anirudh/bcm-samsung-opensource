#
# config.
# -> qcm_dut is copied to /system/bin
#
LOCAL_PATH:= $(call my-dir)

## ifeq ($(BOARD_HAVE_BLUETOOTH_QCOM),true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	qcom_dut.c

LOCAL_CFLAGS:= \
	-DVERSION=\"4.69\" \
	-D__BSD_VISIBLE=1

LOCAL_C_INCLUDES:=\
	external/bluetooth/bluez/lib

LOCAL_SHARED_LIBRARIES := \
	libbluetooth \
	libcutils

LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE:=qcom_dut

include $(BUILD_EXECUTABLE)
## endif

#
# config.
# -> qcm_audio_dut is copied to /system/bin
#

#
# qcm_audio_dut
#
## ifeq ($(BOARD_HAVE_BLUETOOTH_QCOM),true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	qcom_audio_dut.c

LOCAL_CFLAGS:= \
	-DVERSION=\"4.69\" \
	-D__BSD_VISIBLE=1

LOCAL_C_INCLUDES:=\
	external/bluetooth/bluez/lib

LOCAL_SHARED_LIBRARIES := \
	libbluetooth \
	libcutils

LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE:=qcom_audio_dut

include $(BUILD_EXECUTABLE)

## endif
