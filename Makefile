# paths
MODULES = src
INCLUDE = include
BUILD = build
BIN = bin

# compiler
CC = g++

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CPPFLAGS = -g -Wall -Werror -lm -I$(INCLUDE)

# Αρχεία .o
OBJS = $(BUILD)/MySort.o $(BUILD)/RecordReader.o

# Το εκτελέσιμο πρόγραμμα
EXEC = $(BIN)/MySort

# Παράμετροι για δοκιμαστική εκτέλεση
ARGS =  -i voters100000.bin -k 4 -e1 $(BIN)/QuickSort -e2 $(BIN)/MergeSort

# Default target
all: $(EXEC) $(BIN)/QuickSort $(BIN)/MergeSort

# Rules
$(BUILD)/%.o: $(MODULES)/%.cpp | $(BUILD)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

$(BIN):
	mkdir -p $(BIN)

$(EXEC): $(OBJS) | $(BIN)
	$(CC) $(OBJS) -o $(EXEC) $(CPPFLAGS)

$(BIN)/QuickSort: $(MODULES)/Sort.cpp $(MODULES)/RecordReader.cpp $(MODULES)/Sorter.cpp | $(BIN)
	$(CC) -D ALG=1 $(CPPFLAGS) $^ -o $@

$(BIN)/MergeSort: $(MODULES)/Sort.cpp $(MODULES)/RecordReader.cpp $(MODULES)/Sorter.cpp | $(BIN)
	$(CC) -D ALG=2 $(CPPFLAGS) $^ -o $@

run: all
	./$(EXEC) $(ARGS)

clean:
	rm -f $(BUILD)/*.o $(BIN)/* debug.txt

debug: $(EXEC)
	valgrind ./$(EXEC) $(ARGS)

.PHONY: clean run debug all
