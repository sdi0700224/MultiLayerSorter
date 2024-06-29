# How To

## Project description

Project has nested forks for second layer of Processes. It uses one pipe for every child and so do the children with each sorter.
Reading of file is been done once in main to get Record num and one time in each sorter, using the range that belongs to it.
I made a common file for Sorting, but with different defines in Makefile, so it uses different algorythm each time.
No handle for edgecases that load is less than 1 for all sorters, just throwing an error. (Sorry for that but I code ~80 hours this week, please be kind)
Checked with valgring.

RecordReader: Does the file reading job plus compare function
Sorter: Implements the 2 algorithms
Sort.cpp: sorter excec(different depenting ALG define). Made it this way to avoid code repetition
MySort.cpp: the main function, does the forking and load splitting, both for root and first layer childer(plitters/mergers)

Tried to code clean, but no so familiar with linux syscalls, so sometimes I was not so cautious, though some functionallity
could be moved to seperate classes so code could potentially be more sustainable. Nevermind this is not gonna be used for production anyway.
Used low level syscall printing just for final results.

## Makefile

Added Dubug command to use valgrid
Clean removes files