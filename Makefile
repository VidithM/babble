.PHONY: all
.DEFAULT_GOAL := all
BUILD_DIR := ./build
INCLUDE_DIRS := -I./include -I./include/compile -I./include/intrinsics
CFLAGS :=
BABBLE_SOURCES := $(wildcard ./src/*.c ./src/compile/*.c)
DATE := `date +"%Y-%m-%d"`
TIME := `date +"%H:%M"`

all: CFLAGS += -DBUILD_DATE=\"$(DATE)\" -DBUILD_TIME=\"$(TIME)\"
ifeq ($(BABBLE_STATIC), 1)
    all: CFLAGS += -static
endif
all:
	@$(CC) -std=c11 $(CFLAGS) -o $(BUILD_DIR)/babble $(INCLUDE_DIRS) $(BABBLE_SOURCES)
	
debug: CFLAGS += -O0 -g -DDEBUG
debug: all

static: CFLAGS += -static
static: all

install:
	@sudo cp $(BUILD_DIR)/babble /usr/local/bin

clean:
	@rm -rf build/*
