# BMS Simulator - Makefile
# Targets: all, run, test, clean

CC        := gcc
CXX       := g++
CFLAGS    := -std=c99 -Wall -Wextra -Wpedantic -Iinclude -MMD -MP
CXXFLAGS  := -std=c++17 -Wall -Wextra -Iinclude -MMD -MP
LDLIBS    := -lm
TEST_LIBS := -lgtest -lgtest_main -pthread

BUILD     := build
TARGET    := can_decoder
TEST_BIN  := $(BUILD)/run_tests
INPUT     ?= data/sample_can_log.csv

SRCS      := $(wildcard src/*.c)
OBJS      := $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS))
LIB_OBJS  := $(filter-out $(BUILD)/main.o,$(OBJS))
TEST_SRCS := $(wildcard tests/*.cpp)
TEST_OBJS := $(patsubst tests/%.cpp,$(BUILD)/tests/%.o,$(TEST_SRCS))

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Tests link every module except main.c
$(TEST_BIN): $(TEST_OBJS) $(LIB_OBJS)
	$(CXX) $^ -o $@ $(TEST_LIBS) $(LDLIBS)

run: all
	./$(TARGET) $(INPUT)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD) $(TARGET)

-include $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)
