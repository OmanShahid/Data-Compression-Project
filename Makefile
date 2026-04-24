CC = gcc
CFLAGS = -Wall -O2

ifeq ($(OS),Windows_NT)
TARGET = stage1_simple.exe
RM = del /Q
else
TARGET = stage1_simple
RM = rm -f
endif

all: $(TARGET)

$(TARGET): stage1_simple.c
	$(CC) $(CFLAGS) -o $(TARGET) stage1_simple.c

clean:
	-$(RM) $(TARGET)

windows:
	x86_64-w64-mingw32-gcc $(CFLAGS) -o stage1_simple.exe stage1_simple.c
	