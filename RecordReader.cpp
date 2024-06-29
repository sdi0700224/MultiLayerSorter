#include "RecordReader.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "debug.h"

using namespace std;

RecordReader::RecordReader(const char* fileName)
{
	FileName = new char[strlen(fileName) + 1];
    strcpy(FileName, fileName);
}

RecordReader::~RecordReader()
{
	delete[] FileName;
}

Record* RecordReader::GetRecords(int& numOfrecords)
{
    int fd;
    Record rec;
    off_t lSize;

    fd = open(FileName, O_RDONLY);
    if (fd == -1)
    {
        perror("Cannot open binary file");
        return nullptr;
    }

    lSize = lseek(fd, 0, SEEK_END);
    numOfrecords = lSize / sizeof(rec);
    cout << endl << "Records found in file: " << numOfrecords << endl << endl;

    return GetRecordsInRange(0, numOfrecords - 1);
}

Record* RecordReader::GetRecordsInRange(int startIndex, int endIndex)
{
    if (startIndex < 0 || endIndex < startIndex) 
    {
        cerr << "Invalid range specified." << endl;
        return nullptr;
    }

    // Open the file independently for each process.
    int fd = open(FileName, O_RDONLY);
    if (fd == -1) 
    {
        perror("Cannot open binary file");
        return nullptr;
    }

    int numRecords = endIndex - startIndex + 1;
    Record* records = new Record[numRecords];
    if (records == nullptr) 
    {
        cerr << "Memory allocation failed" << endl;
        close(fd);
        return nullptr;
    }

    // Use pread to read the specified range of records.
    ssize_t bytesRead = pread(fd, records, numRecords * sizeof(Record), startIndex * sizeof(Record));
    if (bytesRead < 0) 
    {
        perror("Error reading records");
        delete[] records;
        close(fd);
        return nullptr;
    }

    close(fd);
    return records;
}

bool RecordReader::CompareRecords(const Record& a, const Record& b) 
{
    int lastNameComparison = strcmp(a.LastName, b.LastName);
    if (lastNameComparison != 0) 
    {
        return lastNameComparison < 0;
    }

    int firstNameComparison = strcmp(a.FirstName, b.FirstName);
    if (firstNameComparison != 0) 
    {
        return firstNameComparison < 0;
    }

    return a.custid < b.custid;
}