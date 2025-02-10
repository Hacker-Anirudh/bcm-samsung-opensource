LOCAL_PATH:= $(call my-dir)

# Relative path from <mydroid> to ril
RIL_BT_INC_ROOT_PATH := $(LOCAL_PATH)/../../../../hardware/ril

# sap plugin

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main.c \
	manager.c \
	server.c \
	sap-dummy.c
	#sap-dummy.c

LOCAL_CFLAGS:= \
	-DVERSION=\"4.93\" \

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../btio \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../src \
	$(LOCAL_PATH)/../gdbus \
	$(call include-path-for, glib) \
	$(call include-path-for, dbus) \
	$(RIL_BT_INC_ROOT_PATH)/include/telephony/ #\
#	$(RIL_BT_INC_ROOT_PATH)/libsecril-client/ \
#	$(RIL_BT_INC_ROOT_PATH)/secril_multi/

LOCAL_SHARED_LIBRARIES := \
	libbluetoothd \
	libbluetooth \
	libbtio \
	libcutils \
	libdbus \
	libglib \
#	libsecril-client \

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/bluez-plugin
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/bluez-plugin
LOCAL_MODULE := sap
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
