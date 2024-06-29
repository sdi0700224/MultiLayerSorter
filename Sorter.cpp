#include "Sorter.h"
#include <iostream>

using namespace std;

void Sorter::MergeSort(Record arr[], int const begin, int const end)
{
    if (DEBUG == 1)
    {
        cout << "Using MergeSort" << endl;
    }
    if (begin >= end)
    {
        return; // Returns recursively
    }

    auto mid = begin + (end - begin) / 2;
    MergeSort(arr, begin, mid);
    MergeSort(arr, mid + 1, end);
    Merge(arr, begin, mid, end);
}

void Sorter::QuickSort(Record arr[], int low, int high) 
{
    if (DEBUG == 1)
    {
        cout << "Using QuickSort" << endl;
    }
    if (low < high) 
    {
        int pi = Partition(arr, low, high);
        QuickSort(arr, low, pi - 1);
        QuickSort(arr, pi + 1, high);
    }
}

void Sorter::Merge(Record arr[], int const left, int const mid, int const right)
{
    auto const subArrayOne = mid - left + 1;
    auto const subArrayTwo = right - mid;

    // Create temp arrays
    Record* leftArray = new Record[subArrayOne];
    Record* rightArray = new Record[subArrayTwo];

    // Copy data to temp arrays
    for (int i = 0; i < subArrayOne; i++)
    {
        leftArray[i] = arr[left + i];
    }
    for (int j = 0; j < subArrayTwo; j++)
    {
        rightArray[j] = arr[mid + 1 + j];
    }        

    int indexOfSubArrayOne = 0; // Initial index of first sub-array
    int indexOfSubArrayTwo = 0; // Initial index of second sub-array
    int indexOfMergedArray = left; // Initial index of merged array

    // Merge the temp arrays back into arr[left..right]
    while (indexOfSubArrayOne < subArrayOne && indexOfSubArrayTwo < subArrayTwo)
    {
        if (RecordReader::CompareRecords(leftArray[indexOfSubArrayOne], rightArray[indexOfSubArrayTwo]))
        {
            arr[indexOfMergedArray] = leftArray[indexOfSubArrayOne];
            indexOfSubArrayOne++;
        }
        else
        {
            arr[indexOfMergedArray] = rightArray[indexOfSubArrayTwo];
            indexOfSubArrayTwo++;
        }
        indexOfMergedArray++;
    }
    // Copy the remaining elements of left[], if there are any
    while (indexOfSubArrayOne < subArrayOne) 
    {
        arr[indexOfMergedArray] = leftArray[indexOfSubArrayOne];
        indexOfSubArrayOne++;
        indexOfMergedArray++;
    }
    // Copy the remaining elements of right[], if there are any
    while (indexOfSubArrayTwo < subArrayTwo) 
    {
        arr[indexOfMergedArray] = rightArray[indexOfSubArrayTwo];
        indexOfSubArrayTwo++;
        indexOfMergedArray++;
    }

    delete[] leftArray;
    delete[] rightArray;
}

int Sorter::Partition(Record arr[], int low, int high)
{
    Record pivot = arr[high];
    int i = low - 1;

    for (int j = low; j <= high - 1; j++) 
    {
        if (RecordReader::CompareRecords(arr[j], pivot))
        {
            i++;
            SwapRecords(arr[i], arr[j]);
        }
    }
    SwapRecords(arr[i + 1], arr[high]);
    return (i + 1);
}

void Sorter::SwapRecords(Record& a, Record& b)
{
    Record temp = a;
    a = b;
    b = temp;
}