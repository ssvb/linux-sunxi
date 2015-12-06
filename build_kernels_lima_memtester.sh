#!/bin/sh

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- mrproper
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- sun4i_lima_memtester_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j4 uImage || exit 1

mv arch/arm/boot/uImage uImage-3.4-sun4i-lima-memtester

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- mrproper
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- sun5i_lima_memtester_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j4 uImage || exit 1

mv arch/arm/boot/uImage uImage-3.4-sun5i-lima-memtester

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- mrproper
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- sun7i_lima_memtester_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j4 uImage || exit 1

mv arch/arm/boot/uImage uImage-3.4-sun7i-lima-memtester
