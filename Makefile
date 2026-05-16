CC = gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude -std=c11
TARGET = bzip2_impl
SOURCES = src/main.c src/block.c src/rle.c src/bwt.c src/mtf.c src/rans.c src/config.c
OBJECTS = $(SOURCES:.c=.o)
RM ?= rm -f

ifeq ($(OS),Windows_NT)
    EXE = .exe
else
    EXE =
endif
RUN_PREFIX = ./

all: $(TARGET)$(EXE)

$(TARGET)$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) src/*.o
	$(RM) $(TARGET) $(TARGET).exe
	$(RM) output.bzp restored.txt stage_log.txt *.stages.txt

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

.PHONY: all clean windows compress decompress test_pipeline
