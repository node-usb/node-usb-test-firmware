PROJECT = firmware
BUILD_DIR = bin
CFILES = main.c
DEVICE = stm32f103x8
OPENCM3_DIR = libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
