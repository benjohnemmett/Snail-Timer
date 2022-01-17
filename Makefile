# Makefile to build light_ws2812 library examples
# This is not a very good example of a makefile - the dependencies do not work, therefore everything is rebuilt every time.

# Change these parameters for your device

F_CPU = 8000000
DEVICE = atmega328p

# Tools:
CC = avr-gcc

LIGHT_LIB = light_ws2812/light_ws2812_AVR

LIB       = light_ws2812
EXAMPLES  = SnailClock
DEP		  = $(LIGHT_LIB)/ws2812_config.h $(LIGHT_LIB)/Light_WS2812/light_ws2812.h

CFLAGS = -g2 -I. -ILight_WS2812 -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) 
CFLAGS+= -Os -ffunction-sections -fdata-sections -fpack-struct -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions  
CFLAGS+= -Wall -Wno-pointer-to-int-cast
#CFLAGS+= -Wa,-ahls=$<.lst

LDFLAGS = -Wl,--relax,--section-start=.text=0,-Map=main.map

all:	$(EXAMPLES) 

$(LIB): $(DEP)
	@echo Building Library 
	@$(CC) $(CFLAGS) -o Objects/$@.o -c $(LIGHT_LIB)/Light_WS2812/$@.c 

$(EXAMPLES): $(LIB) 
	@echo Building $@
	@$(CC) $(CFLAGS) -o Objects/$@.o $@.c $(LIGHT_LIB)/Light_WS2812/$^.c
	@avr-size Objects/$@.o
	@avr-objcopy -j .text  -j .data -O ihex Objects/$@.o $@.hex
	@avr-objdump -d -S Objects/$@.o >Objects/$@.lss

.PHONY:	clean

flash: 
	avrdude -p atmega328p -c usbasp -U flash:w:SnailClock.hex

clean:
	rm -f *.hex Objects/*.o Objects/*.lss
