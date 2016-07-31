## Control firmware

This program is written using the ARM mbed tools for an STM32 Nucleo board (STM32F303RE at the moment).

Some notes:
* I prefer to work locally over using the mbed online IDE. I exported a basic example program from mbed.org for the GNU GCC ARM toolchain to get a starter Makefile and the mbed/ directory containing the resources for my board. The latter is not under version control.
* Currently depends on the [LidarLitev2](https://developer.mbed.org/users/sventura3/code/LidarLitev2/) library and my fork of the [BNO055](https://github.com/andrewadare/BNO055.git) absolute orientation sensor library. The directories containing those sources need to be copied or linked here (or paths modified in the Makefile). The former is not actually being used at the moment, and may be removed.
