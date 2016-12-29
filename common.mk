include message.mk

# Directory common source files exist
TOP_DIR = .
TMK_DIR = tmk_core
TMK_PATH = $(TOP_DIR)/$(TMK_DIR)
LIB_PATH = $(TOP_DIR)/lib

QUANTUM_DIR = quantum
QUANTUM_PATH = $(TOP_DIR)/$(QUANTUM_DIR)

BUILD_DIR := $(TOP_DIR)/.build

SERIAL_DIR := $(QUANTUM_DIR)/serial_link
SERIAL_PATH := $(QUANTUM_PATH)/serial_link
SERIAL_SRC := $(wildcard $(SERIAL_PATH)/protocol/*.c)
SERIAL_SRC += $(wildcard $(SERIAL_PATH)/system/*.c)
SERIAL_DEFS += -DSERIAL_LINK_ENABLE

API_DIR := $(QUANTUM_DIR)/api
API_PATH := $(QUANTUM_PATH)/api
API_SRC := $(wildcard $(API_PATH)/*.c)
API_DEFS += -DAPI_ENABLE

API2_DIR := $(QUANTUM_DIR)/apiv2
API2_PATH := $(QUANTUM_PATH)/apiv2
API2_SRC := $(wildcard $(API2_PATH)/*.c)
API2_DEFS += -DAPI2_ENABLE

APISYSEX_DIR := $(QUANTUM_DIR)/api/sysex
APISYSEX_PATH := $(QUANTUM_PATH)/api/sysex
APISYSEX_SRC := $(wildcard $(APISYSEX_PATH)/*.c)
APISYSEX_DEFS += -DAPI_SYSEX_ENABLE

COMMON_VPATH := $(TOP_DIR)
COMMON_VPATH += $(TMK_PATH)
COMMON_VPATH += $(QUANTUM_PATH)
COMMON_VPATH += $(QUANTUM_PATH)/keymap_extras
COMMON_VPATH += $(QUANTUM_PATH)/audio
COMMON_VPATH += $(QUANTUM_PATH)/process_keycode
COMMON_VPATH += $(SERIAL_PATH)