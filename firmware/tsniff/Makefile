PACKAGE = tsniff.bix

XDATA_START = 0x1000

ASFLGS = -olsxff
CFLGS = --opt-code-speed -mmcs51 -DXDATA_START=$(XDATA_START)
LDFLGS = --xram-loc $(XDATA_START)

INCLUDE_DIR = ../include
LIB_DIR = ../lib/build
BUILD_DIR = build
CC = sdcc

HEX2BIX = objcopy -I ihex -O binary

SRCS_C = \
	fw.c main.c timer.c
SRCS_ASM = \
	dscr.asm USBJmpTb.asm

OBJS = $(SRCS_C:.c=.rel) $(SRCS_ASM:.asm=.rel)

all : $(BUILD_DIR) $(BUILD_DIR)/$(PACKAGE) $(PACKAGE:.bix=.inc)

$(BUILD_DIR)/%.rel : %.c
	$(CC) -c $(CFLGS) -I$(INCLUDE_DIR) -o $@ $<

$(BUILD_DIR)/%.rel : %.asm
	cp $< $(BUILD_DIR)
	cd $(BUILD_DIR)
	asx8051 -losg $(BUILD_DIR)/$<

$(BUILD_DIR)/$(PACKAGE:.bix=.ihx) : $(patsubst %,$(BUILD_DIR)/%,$(OBJS))
	$(CC) $(LDFLGS) -o $@ -L$(LIB_DIR) -l ezusb $^

$(BUILD_DIR)/%.hex : $(BUILD_DIR)/%.ihx
	cp $< $@

$(BUILD_DIR)/$(PACKAGE) : $(BUILD_DIR)/$(PACKAGE:.bix=.hex)
	$(HEX2BIX) --pad-to 8192 $< $(BUILD_DIR)/$(PACKAGE)

$(PACKAGE:.bix=.inc): $(BUILD_DIR)/$(PACKAGE)
	../bin2h.py $< >$@

$(BUILD_DIR) :
	mkdir $@

clean :
	rm -f $(BUILD_DIR)/*

run : all

.PHONY : clean all
