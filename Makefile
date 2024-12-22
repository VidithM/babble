.PHONY: all
.DEFAULT_GOAL := all
BUILD_DIR := ./build
INCLUDE_DIRS := ./include
CFLAGS :=
BABBLE_SOURCES := $(wildcard ./src/*.c)

all:
	@gcc $(CFLAGS) -o $(BUILD_DIR)/babble -I$(INCLUDE_DIRS) $(BABBLE_SOURCES)
	
debug: CFLAGS += -g 
debug: all

clean:
	@rm -rf build/*