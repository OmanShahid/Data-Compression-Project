CC = gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude -std=c11
TARGET = bzip2_impl
SOURCES = src/main.c src/block.c src/rle.c src/bwt.c src/mtf.c src/rans.c src/config.c
OBJECTS = $(SOURCES:.c=.o)

ifeq ($(OS),Windows_NT)
    EXE = .exe
    RUN_PREFIX = .\
else
    EXE =
    RUN_PREFIX = ./
endif

# =====================================================================
# DEFAULT TARGET (Moved to the top so 'make' runs this by default)
# =====================================================================
default: test_pipeline

all: $(TARGET)$(EXE)

$(TARGET)$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) src\*.o
	$(RM) $(TARGET)$(EXE)

windows:
	x86_64-w64-mingw32-gcc $(CFLAGS) -o $(TARGET).exe $(SOURCES)

# =====================================================================
# AUTOMATION TARGETS 
# =====================================================================

# Shortcut to run compression
compress: $(TARGET)$(EXE)
	$(RUN_PREFIX)$(TARGET)$(EXE) c input.txt output.bzp config.ini

# Shortcut to run decompression
decompress: $(TARGET)$(EXE)
	$(RUN_PREFIX)$(TARGET)$(EXE) d output.bzp restored.txt

# Shortcut to do both back-to-back
test_pipeline: compress decompress

.PHONY: default all clean windows compress decompress test_pipeline