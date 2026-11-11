# Offline Relief Camp Manager - C++17, standard library only.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -Iinclude
LDFLAGS  ?= -pthread

SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests
BUILD_DIR := build
BIN_DIR   := bin

APP_MAIN  := $(SRC_DIR)/main.cpp
SEED_MAIN := $(SRC_DIR)/seed_main.cpp
ALL_SRC   := $(wildcard $(SRC_DIR)/*.cpp)
LIB_SRC   := $(filter-out $(APP_MAIN) $(SEED_MAIN),$(ALL_SRC))
TEST_SRC  := $(wildcard $(TEST_DIR)/*.cpp)

APP_BIN   := $(BIN_DIR)/relief
TEST_BIN  := $(BIN_DIR)/tests
SEED_BIN  := $(BIN_DIR)/seed

.PHONY: all test debug seed clean run
all: $(APP_BIN)

$(APP_BIN): $(LIB_SRC) $(APP_MAIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) $(APP_MAIN) -o $@ $(LDFLAGS)

$(TEST_BIN): $(LIB_SRC) $(TEST_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -I$(TEST_DIR) $(LIB_SRC) $(TEST_SRC) -o $@ $(LDFLAGS)

$(SEED_BIN): $(LIB_SRC) $(SEED_MAIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) $(SEED_MAIN) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

debug: CXXFLAGS += -DRELIEF_DEBUG -g
debug: clean $(APP_BIN)

seed: $(SEED_BIN)
	./$(SEED_BIN)

run: $(APP_BIN)
	./$(APP_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
