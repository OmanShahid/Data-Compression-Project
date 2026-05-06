CC = gcc
# Added -std=c11 to the line below
CFLAGS ?= -Wall -Wextra -O2 -Iinclude -std=c11
TARGET = bzip2_impl
SOURCES = src/main.c src/block.c src/rle.c src/bwt.c src/mtf.c src/rans.c src/config.c
OBJECTS = $(SOURCES:.c=.o)

ifeq ($(OS),Windows_NT)
    EXE = .exe
else
    EXE =
endif

all: $(TARGET)$(EXE)

$(TARGET)$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) src/*.o
	$(RM) $(TARGET)$(EXE)

windows:
	x86_64-w64-mingw32-gcc $(CFLAGS) -o $(TARGET).exe $(SOURCES)

.PHONY: all clean windows