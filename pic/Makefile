SOURCES = $(wildcard *.c)
APP_LIB = /opt/microchip/app-lib/Microchip
SOURCES += $(APP_LIB)/USB/usb_device.c "$(APP_LIB)/USB/CDC Device Driver/usb_function_cdc.c"
DEPENDENCIES = $(wildcard $(SOURCES) *.h)
TARGET = program
CHIP = 16F1454
CC = xc8
CFLAGS += --chip=$(CHIP)
CFLAGS += -G
CFLAGS += -I. -I$(APP_LIB)/Include
CFLAGS += --errformat="%f:%l: error: %s" --warnformat="%f:%l: warning: %s" --msgformat="%f:%l: message: %s"

build/$(TARGET).hex: $(wildcard $(SOURCES) $(DEPENDENCIES))
	@mkdir -p build
	$(CC) $(CFLAGS) $(SOURCES) --outdir=build -o$(TARGET)
	@mv funclist l.obj build

-include $(wildcard build/*.d)

.PHONY: clean upload

clean:
	rm -rf build

upload: build/$(TARGET).hex
	picp program < $<

