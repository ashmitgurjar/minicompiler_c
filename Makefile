CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O0 -g

BUILD := build
BIN := $(BUILD)/minic

PARSER_CPP := $(BUILD)/parser.cpp
PARSER_HPP := $(BUILD)/parser.hpp
SCANNER_CPP := $(BUILD)/lexer.cpp

SRCS := \
  src/ast.cpp \
  src/print.cpp \
  src/sema.cpp \
  src/symbol_table.cpp \
  src/interpreter.cpp \
  src/optimizer.cpp \
  src/main.cpp

OBJS := $(SRCS:%.cpp=$(BUILD)/%.o) \
        $(BUILD)/parser.o \
        $(BUILD)/lexer.o

.PHONY: all clean run

all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD) $(BUILD)/src

$(PARSER_CPP) $(PARSER_HPP): parser.y | $(BUILD)
	bison -d -o $(PARSER_CPP) --defines=$(PARSER_HPP) parser.y

$(SCANNER_CPP): lexer.l $(PARSER_HPP) | $(BUILD)
	flex -o $(SCANNER_CPP) lexer.l

$(BUILD)/parser.o: $(PARSER_CPP) $(PARSER_HPP) | $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(BUILD) -Isrc -c $(PARSER_CPP) -o $@

$(BUILD)/lexer.o: $(SCANNER_CPP) $(PARSER_HPP) | $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(BUILD) -Isrc -c $(SCANNER_CPP) -o $@

$(BUILD)/src/%.o: src/%.cpp src/%.hpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(BUILD) -Isrc -c $< -o $@

$(BUILD)/src/main.o: src/main.cpp src/ast.hpp src/sema.hpp $(PARSER_HPP) | $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(BUILD) -Isrc -c $< -o $@

$(BIN): $(OBJS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

run: $(BIN)
	./$(BIN) examples/ok.mc

clean:
	rm -rf $(BUILD)

