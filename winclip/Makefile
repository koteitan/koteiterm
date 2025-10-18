# Makefile for winclip.exe - Windows Clipboard Utility

CC = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -O2
TARGET = winclip.exe

all: $(TARGET)

$(TARGET): winclip.c
	$(CC) $(CFLAGS) -o $(TARGET) winclip.c

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) ..

.PHONY: all clean install
