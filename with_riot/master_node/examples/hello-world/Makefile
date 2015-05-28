# name of your application
APPLICATION = hello-world

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:

INCLUDES += -I../../sdk7.2/components/toolchain/gcc
INCLUDES += -I../../sdk7.2/components/toolchain
INCLUDES += -I../../sdk7.2/components/libraries/button
INCLUDES += -I../../sdk7.2/components/libraries/sensorsim
INCLUDES += -I../../sdk7.2/components/ble/ble_services/ble_hrs
INCLUDES += -I../../sdk7.2/components/ble/ble_services/ble_dis
INCLUDES += -I../../sdk7.2/components/ble/ble_services/ble_bas
INCLUDES += -I../../sdk7.2/components/softdevice/s110/headers
INCLUDES += -I../../sdk7.2/components/ble/common
INCLUDES += -I../../sdk7.2/components/libraries/timer
INCLUDES += -I../../sdk7.2/components/libraries/gpiote
INCLUDES += -I../../sdk7.2/components/drivers_nrf/hal
INCLUDES += -I../../sdk7.2/components/ble/device_manager
INCLUDES += -I../../sdk7.2/components/ble/device_manager/config
INCLUDES += -I../../sdk7.2/components/softdevice/common/softdevice_handler
INCLUDES += -I../../sdk7.2/components/libraries/scheduler
INCLUDES += -I../../sdk7.2/components/drivers_nrf/pstorage
INCLUDES += -I../../sdk7.2/components/drivers_nrf/pstorage/config
INCLUDES += -I../../sdk7.2/components/libraries/util
INCLUDES += -I../../sdk7.2/components/libraries/trace
INCLUDES += -I../../sdk7.2/components/drivers_nrf/uart
INCLUDES += -I../../sdk7.2/components/libraries/fifo
INCLUDES += -I../../sdk7.2/examples/bsp

CFLAGS += -DNRF51
CFLAGS += -DBLE_STACK_SUPPORT_REQD
CFLAGS += -DS110
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
