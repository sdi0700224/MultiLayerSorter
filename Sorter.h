#pragma once
#include "RecordReader.h"
#include "Debug.h"

enum SortingAlgorithms
{
	MergeSort,
    QuickSort
};

class Sorter
{
private:
    static void Merge(Record arr[], int const left, int const mid, int const right);
    static int Partition(Record arr[], int low, int high);
    static void SwapRecords(Record& a, Record& b);

public:
    static void MergeSort(Record arr[], int const begin, int const end);
    static void QuickSort(Record arr[], int const begin, int const end);
};


