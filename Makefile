#!/usr/bin/make -f
DIST     = .
CC       = gcc
CFLAGS   = -O2 -g -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 `pkg-config --cflags glib-2.0` -Ilibusb/libusb
LDFLAGS  = 

OBJS   = cusbfx2.o
LIBS   = `pkg-config --libs glib-2.0` libusb/libusb/.libs/libusb-1.0.a -lrt
TARGET = $(DIST)/recfx2

all: $(TARGET) loadfx2fw

clean:
	rm -f $(OBJS) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

cusbfx2.o: cusbfx2.c cusbfx2.h
