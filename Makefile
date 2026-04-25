CC = gcc
CFLAGS = -Wall -O2

ifeq ($(OS),Windows_NT)
TARGET = stage1_simple.exe
RM = del /Q
RUN = .\\$(TARGET)
else
TARGET = stage1_simple
RM = rm -f
RUN = ./$(TARGET)
endif

.PHONY: all run clean windows

all: run

$(TARGET): stage1_simple.c
	$(CC) $(CFLAGS) -o $(TARGET) stage1_simple.c

run: $(TARGET)
	$(RUN) input.txt output.txt config.ini

clean:
	-$(RM) $(TARGET)

windows:
	x86_64-w64-mingw32-gcc $(CFLAGS) -o stage1_simple.exe stage1_simple.c
	