1. How to Build
	- get Toolchain
		From android source download site (http://source.android.com/source/downloading.html)
		Toolchain is included in android source code.

	- edit "Makefile"
		edit "CROSS_COMPILE" to right toolchain path(You downloaded).
		ex) CROSS_COMPILE=/opt/toolchains/arm-eabi-4.6/bin/arm-eabi-

	- make
		$ ./build_kernel.sh

2. Output files
	- Kernel : kernel_out/arch/arm/boot/zImage
	- module : kernel_out/*/*.ko

3. How to make .tar binary for downloading into target.
	- change current directory to Kernel/arch/arm/boot
	- type following command
	$ tar cvf GT-S6293T_LA_ZTO_Kernel.tar zImage