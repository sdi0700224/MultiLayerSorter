#include <iostream>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <sys/times.h>
#include "RecordReader.h"
#include "Sorter.h"
#include "Debug.h"


using namespace std;

ssize_t writeToPipe(int fd, void *buf, size_t totalSize);

int main(int argc, char* argv[]) 
{
    if (argc != 6) 
    {
        cerr << "Usage: " << argv[0] << " <file> <start_index> <end_index> <output_file> <record_size>" << endl;
        return 1;
    }

    const char* filename = argv[1];
    int startIndex = atoi(argv[2]);
    int endIndex = atoi(argv[3]);
    int pipeFd = atoi(argv[4]);
    int rootPid = atoi(argv[5]);

    RecordReader reader(filename);

    double t1, t2, cpu_time;
    struct tms tb1, tb2;
    double ticspersec;
    ticspersec = (double)sysconf(_SC_CLK_TCK);
    t1 = (double)times(&tb1);


    if (DEBUG == 1)
    {
        cout << "Starting: " << startIndex << " Ending: " << endIndex << endl;
    }
    
    Record* records = reader.GetRecordsInRange(startIndex, endIndex);

    if (!records) 
    {
        cerr << "Error reading records." << endl;
        return 1;
    }

    #ifdef ALG
        #if ALG == 1
            Sorter::QuickSort(records, 0, endIndex - startIndex);
        #elif ALG == 2
            Sorter::MergeSort(records, 0, endIndex - startIndex);
        #else
            std::cout << "ALG defined to an unknown value" << std::endl;
        #endif
    #else
        std::cout << "ALG is not defined" << std::endl;
    #endif

    if (DEBUG == 1)
    {
        cout << "Starting: " << startIndex << " Ending: " << endIndex << " Sorting finished" << endl;
        for (int i = 0; i <= endIndex - startIndex; i++)
        {
            printf("%d %s %s  %s \n", records[i].custid, records[i].LastName, records[i].FirstName, records[i].postcode);
        }
    }

    writeToPipe(pipeFd, (void*)records, sizeof(Record) * (endIndex - startIndex + 1));

    t2 = (double)times(&tb2);
    cpu_time = (double)((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime));
    //printf ("Run time was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n", (t2 - t1) / ticspersec, cpu_time / ticspersec);

    double times[2];
    times[0] = (t2 - t1) / ticspersec;
    times[1] = cpu_time / ticspersec;

    writeToPipe(pipeFd, (void*)times, sizeof(double) * 2);

    close(pipeFd);

    delete[] records;

    kill(rootPid, SIGUSR2);
    return EXIT_SUCCESS;
}

ssize_t writeToPipe(int fd, void *buf, size_t totalSize)
{
    size_t totalBytesWrite = 0;

    while (true) 
    {
        ssize_t bytesWrite = write(fd, static_cast<char*>(buf) + totalBytesWrite, totalSize - totalBytesWrite);

        if (bytesWrite == -1)
        {
            cerr << "Error writing to pipe: " << errno << endl;
            if (errno == EINTR || errno == 0)
            {
                continue; // Retry writing if interrupted
            }
            return -1;
        }

        totalBytesWrite += bytesWrite;
        
        // Check if all data is read
        if (totalBytesWrite >= totalSize)
        {
            if (DEBUG)
            {
                cout << "Writing finished for pipe" << endl;
            }
            break;
        }
        else
        {
            if (DEBUG)
            {
                cout << "Read: " << totalBytesWrite << " out of: " << totalSize << endl;
            }
        }
    }

    return totalBytesWrite;
}