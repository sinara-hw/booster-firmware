# booster-firmware

Firmware of RFPA Booster device.
https://github.com/sinara-hw/Booster

# Build Instructions
Project files are part of SystemWorkbench for STM32 software. In order to build the firmware, please follow steps below:

* Visit [Link](http://www.openstm32.org/System%2BWorkbench%2Bfor%2BSTM32), register and download installer
* Install SW4STM32
* File -> Import -> General -> Existing Projects into Workspace
* Select project (eth-scpi) from Project Explorer
* Click __Hammer__ icon in order to build firmware

# Preparing DFU file for bootloader flashing
* Make sure you have `arm-none-eabi-gcc` toolchain installed
* Download [dfuse-tool](https://github.com/plietar/dfuse-tool) from Github
* Execute following steps in Debug directory after building the firmware:
  1. `arm-none-eabi-objcopy -O ihex eth-scpi.elf eth-scpi.hex`
  2. `dfu-convert -i eth-scpi.hex eth-scpi.dfu`
* Flash generated file in bootloader mode (Enter DFU by pressing the DFU button and powering the device)
* By using command `dfu-util -D eth-scpi.dfu -a 0` (valid for most cases, if any error occurred visit official [wiki](https://github.com/sinara-hw/Booster/wiki#firmware-update---linux)
