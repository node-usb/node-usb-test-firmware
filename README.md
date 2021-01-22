# Node.js USB Test Firmware

[![Build Status](https://github.com/thegecko/node-usb-test-firmware/workflows/ci/badge.svg)](https://github.com/thegecko/node-usb-test-firmware/actions)

Test firmware for the [node-usb](https://github.com/tessel/node-usb) project.

## Usage

This project has been built for the [STM32F103 Microprocessor](https://www.st.com/en/microcontrollers-microprocessors/stm32f103.html), specifically the low-cost `blue-pill` device.

Grab a binary from the [releases](https://github.com/thegecko/node-usb-test-firmware/releases) and flash it to your hardware.

Execute the [node-usb tests](https://github.com/tessel/node-usb/blob/master/test/usb.coffee) and ensure they pass.

## Flashing

There are many ways to flash binaries to target hardware, this project was built using an [ST-LINK/v2](https://www.st.com/en/development-tools/st-link-v2.html) on `MacOS` as follows:

Connect `3V3`, `SWDIO`, `SWCLK` and `GND` wires of the SWD connector to the ST-LINK device:

<img src="https://www.programmersought.com/images/160/33d77575620495f71efef4eb4598a3c8.png">

Install the [OSS ST-LINK tools](https://github.com/stlink-org/stlink)

```bash
> brew install stlink
```

Confirm the device can be found:

```bash
> st-info --probe
```

Flash the firmware to the device:

```bash
> st-flash --reset write firmware.bin 0x08000000
```

## Development

If you want to build this locally (for example to target a [different device](https://github.com/libopencm3/libopencm3/blob/master/ld/devices.data)), clone this repository recursively:

```bash
> git clone https://github.com/thegecko/node-usb-test-firmware --recurse-submodules
```

Ensure the arm-embedded gcc toolchain is installed. eg. on `MacOS`:

```bash
> brew install --cask gcc-arm-embedded
```

Build the project:

```bash
> make -C libopencm3
> make
```
