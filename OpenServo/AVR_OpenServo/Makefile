###############################################################################
# Makefile for the project ATTiny44A_OpenServo
###############################################################################

## General Flags
PROJECT = ATTiny44A_OpenServo
MCU = attiny44a
TARGET = ATTiny44A_OpenServo.elf
CC = avr-gcc

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -Os -fsigned-char
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -nostartfiles -v  -Wl,-Map=ATTiny44A_OpenServo.map


## Objects that must be built in order to link
OBJECTS = bootcrt.o main.o adc.o registers.o eeprom.o watchdog.o motion.o math.o ipd.o pid.o regulator.o power.o pwm.o estimator.o timer.o usiTwiSlave.o 

## Objects explicitly added by the user
LINKONLYOBJECTS =

## Build
all: $(TARGET) ATTiny44A_OpenServo.hex ATTiny44A_OpenServo.lss size

## Flash
flash: ATTiny44A_OpenServo.hex
	## Add fuse configurations here!
	sudo avrdude -p t44 -c dragon_isp -Pusb -v -U flash:w:ATTiny44A_OpenServo.hex

## Compile
bootcrt.o: bootcrt.s
	$(CC) $(INCLUDES) $(ASMFLAGS) -c  $<

main.o: main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

adc.o: adc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

registers.o: registers.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

timer.o: timer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

eeprom.o: eeprom.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

watchdog.o: watchdog.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

motion.o: motion.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

math.o: math.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ipd.o: ipd.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

pid.o: pid.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

regulator.o: regulator.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

power.o: power.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

pulsectl.o: pulsectl.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<

twi.o: iusiTwiSlave.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

pwm.o: pwm.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

estimator.o: estimator.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

curve.o: curve.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $< 

seek.o: seek.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) ATTiny44A_OpenServo.elf dep/* ATTiny44A_OpenServo.hex ATTiny44A_OpenServo.lss ATTiny44A_OpenServo.map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

