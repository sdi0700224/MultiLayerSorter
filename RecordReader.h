#pragma once
#include <stdio.h>
#include <unistd.h>

#define SIZEofBUFF 20
#define SSizeofBUFF 6

struct Record
{
	int custid;
	char LastName[SIZEofBUFF];
	char FirstName[SIZEofBUFF];
	char postcode[SSizeofBUFF];
};

class RecordReader
{
private:
	char* FileName;

public:
	RecordReader(const char* fileName);
	~RecordReader();

	Record* GetRecords(int& numOfrecords);
	Record* GetRecordsInRange(int startIndex, int endIndex);
	static bool CompareRecords(const Record& a, const Record& b);
};
