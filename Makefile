# Offline Relief Camp Manager - C++17, standard library only.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -Iinclude
LDFLAGS  ?= -pthread

SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests
BUILD_DIR := build
BIN_DIR   := bin

# Build mode: "release" by default, "debug" when started through `make debug`.
MODE      ?= release
MODE_FILE := $(BUILD_DIR)/.buildmode
ifeq ($(MODE),debug)
CXXFLAGS  += -DRELIEF_DEBUG -g
endif

APP_MAIN  := $(SRC_DIR)/main.cpp
SEED_MAIN := $(SRC_DIR)/seed_main.cpp
ALL_SRC   := $(wildcard $(SRC_DIR)/*.cpp)
LIB_SRC   := $(filter-out $(APP_MAIN) $(SEED_MAIN),$(ALL_SRC))
TEST_SRC  := $(wildcard $(TEST_DIR)/*.cpp)
HEADERS   := $(wildcard $(INC_DIR)/*.h) $(wildcard $(TEST_DIR)/*.h)

APP_BIN   := $(BIN_DIR)/relief
TEST_BIN  := $(BIN_DIR)/tests
SEED_BIN  := $(BIN_DIR)/seed

.PHONY: all test debug seed clean run FORCE
all: $(APP_BIN)

$(APP_BIN): $(LIB_SRC) $(APP_MAIN) $(HEADERS) $(MODE_FILE) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) $(APP_MAIN) -o $@ $(LDFLAGS)

$(TEST_BIN): $(LIB_SRC) $(TEST_SRC) $(HEADERS) $(MODE_FILE) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -I$(TEST_DIR) $(LIB_SRC) $(TEST_SRC) -o $@ $(LDFLAGS)

$(SEED_BIN): $(LIB_SRC) $(SEED_MAIN) $(HEADERS) $(MODE_FILE) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) $(SEED_MAIN) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

# `make debug` runs make again in debug mode. The binary keeps its normal path.
debug:
	$(MAKE) MODE=debug $(APP_BIN)

# The stamp file remembers the mode of the last build. It is rewritten only
# when the mode changes, which makes the binaries out of date and rebuilds them.
$(MODE_FILE): FORCE
	@mkdir -p $(BUILD_DIR)
	@if [ "$$(cat $@ 2>/dev/null)" != "$(MODE)" ]; then echo $(MODE) > $@; fi

FORCE:

seed: $(SEED_BIN)
	./$(SEED_BIN)

run: $(APP_BIN)
	./$(APP_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
