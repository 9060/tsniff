PACKAGE = ezusb.lib

BUILD_DIR = build
INCLUDE_DIR = ../include

SRCS_C = \
	EZRegs.c get_strd.c resume.c \
	discon.c i2c.c \
	delay.c i2c_rw.c
SRCS_ASM = \
	delayms.asm susp.asm

OBJS = $(SRCS_C:.c=.rel) $(SRCS_ASM:.asm=.rel)

$(BUILD_DIR)/%.rel : %.c
	sdcc -c -mmcs51 -I$(INCLUDE_DIR) -o $@ $<

$(BUILD_DIR)/%.rel : %.asm
	cp $< $(BUILD_DIR)/; \
	cd $(BUILD_DIR); \
	asx8051 -losg $<

$(BUILD_DIR)/$(PACKAGE) : $(patsubst %,$(BUILD_DIR)/%,$(OBJS))
	for obj in $^; do sdcclib $@ $$obj; done

$(BUILD_DIR):
	mkdir $@

all : $(BUILD_DIR) $(BUILD_DIR)/$(PACKAGE)

clean :
	rm -f $(PACKAGE) $(BUILD_DIR)/*

run : all

.PHONY : clean all
