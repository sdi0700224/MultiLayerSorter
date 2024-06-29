#paths
MODULES = 
INCLUDE = 

# compiler
CC = g++

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CPPFLAGS = -g -Wall -Werror -lm

# Αρχεία .o
OBJS = MySort.o RecordReader.o

# Το εκτελέσιμο πρόγραμμα
EXEC = MySort

# Παράμετροι για δοκιμαστική εκτέλεση
ARGS =  -i voters100000.bin -k 4 -e1 QuickSort -e2 MergeSort

$(EXEC): $(OBJS) 
	$(CC) $(OBJS) -o $(EXEC) $(CPPFLAGS)
	g++ -o QuickSort -D ALG=1 Sort.cpp RecordReader.cpp Sorter.cpp
	g++ -o MergeSort -D ALG=2 Sort.cpp RecordReader.cpp Sorter.cpp

run: $(EXEC)
	./$(EXEC) $(ARGS)

clean:
	rm -f $(OBJS) $(EXEC) Quicksort MergeSort debug.txt

debug: $(EXEC)
	valgrind ./$(EXEC) $(ARGS)
	ipcs