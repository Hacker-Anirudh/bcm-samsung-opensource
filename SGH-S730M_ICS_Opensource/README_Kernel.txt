########################
##### How to Build #####
########################

(1) Get Toolchain from Codesourcery site. (http://www.codesourcery.com)
    ex) Download : https://sourcery.mentor.com/sgpp/lite/arm/portal/subscription?@template=lite
    recommand - Feature : ARM
                Target OS : "EABI"
                Release : "Sourcery G++ Lite 2009q3-68"
                package : "IA32 GNU/Linux TAR"

(2) Edit Makefile for compile.
    edit "CROSS_COMPILE" to right toolchain path which you downloaded.
    ex) ARCH  ?= arm
        CROSS_COMPILE	?= /opt/toolchains/arm-eabi-4.4.3/bin/arm-eabi-   //You have to modify this path!

(3) Compile as follow commands.
		 make mrproper
		 make ARCH=arm amazing-pref_defconfig
		 make

(4) Get the zImage on follow path.
    kernel/arch/arm/boot/zImage