How to build Module for Platform
- It is only for modules are needed to using Android build system.
- Please check its own install information under its folder for other module.

[Step to build]
1. Get android open source.
	: version info - Android 4.1.2
	(Download site : http://source.android.com)

2. Copy module that you want to build to original android open source. If same module exist in android open source, you should replace it. (no overwrite)
	# It is possible to build all modules at once.

3. In case of 'external\bluetooth', you should add following text in 'build\target\board\generic\BoardConfig.mk'
	BOARD_HAVE_BLUETOOTH := true
	BOARD_HAVE_BLUETOOTH_BCM := true

4. In case of 'device\samsung\bcm_common\alsa-lib', you should add following text in 'build\target\board\generic\BoardConfig.mk'
	OPENSOURCE_ALSA_AUDIO := true
	BOARD_USES_ALSA_AUDIO := true
	BRCM_ALSA_LIB_DIR=device/samsung/bcm_common/alsa-lib

5. You should add module name to 'PRODUCT_PACKAGES' in 'build\target\product\core.mk' as following case.
	case 1) e2fsprog : should add 'e2fsck' to PRODUCT_PACKAGES
	case 2) libexifa : should add 'libexifa' to PRODUCT_PACKAGES
	case 3) libjpega : should add 'libjpega' to PRODUCT_PACKAGES
	case 4) KeyUtils : should add 'libkeyutils' to PRODUCT_PACKAGES
	case 5) bluetooth : should add 'audio.a2dp.default' to PRODUCT_PACKAGES
	case 6) libasound : should add 'libasound' to PRODUCT_PACKAGES
	case 7) bcm_dut : should add 'bcm_dut' to PRODUCT_PACKAGES
	case 8) audio\alsaplugin : should add 'libasound_module_pcm_bcmfilter' to PRODUCT_PACKAGES

	ex) [build\target\product\core.mk] - add all module name for case 1 ~ 8 at once
		PRODUCT_PACKAGES += \
			e2fsck \
			libexifa \
			libjpega \
			libkeyutils \
			audio.a2dp.default \
			libasound \
			libasound_module_pcm_bcmfilter \
			bcm_dut

6. excute build command
	./build.sh user