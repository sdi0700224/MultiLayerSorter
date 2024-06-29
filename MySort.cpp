#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <math.h>
#include <fstream> // Only for debugging
#include "RecordReader.h"
#include "Debug.h"

using namespace std;

void MergeSortedArrays(Record* destination, Record** sourceArrays, int* sourceArraysCounters, int numArrays);
void MergeTimeArrays(double* destination, double** sourceArrays, int numArrays);
void signalHandler(int sigNum);
ssize_t readFromPipe(int fd, void *buf, size_t totalSize);
ssize_t writeToPipe(int fd, void *buf, size_t totalSize);
void writeString(int fd, const char* str);
void writeInt(int fd, int value);
void writeFractional(int fd, double fractional, int decimalPlaces);
void writeDouble(int fd, double value);

int main(int argc, char** argv)
{
    char* dataFile = nullptr;
    int numOfChildren = -1;
    char* sorting1 = nullptr;
    char* sorting2 = nullptr;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
        {
            dataFile = argv[++i];
        }
        else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc)
        {
            numOfChildren = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-e1") == 0 && i + 1 < argc)
        {
            sorting1 = argv[++i];
        }
        else if (strcmp(argv[i], "-e2") == 0 && i + 1 < argc)
        {
            sorting2 = argv[++i];
        }
    }

    if (!dataFile || numOfChildren == -1 || !sorting1 || !sorting2)
    {
        cerr << "Missing required arguments\n";
        return 1;
    }

    RecordReader reader(dataFile);

    signal(SIGUSR1, signalHandler);
    signal(SIGUSR2, signalHandler);
    int numOfRecords;
    Record* records = reader.GetRecords(numOfRecords);

    int** pipes = new int*[numOfChildren]; // Dynamic array of pipes
    for (int i = 0; i < numOfChildren; i++)
    {
        pipes[i] = new int[2]; // Each element is an array of two integers
        if (pipe(pipes[i]) == -1)
        {
            cerr << "Pipe Failed" << endl;
            return 1;
        }
    }

    int recordLoadPerChild = numOfRecords / numOfChildren;
    if (recordLoadPerChild == 0)
    {
        cerr << "Error, load should be at least 1 Record / child" << endl;
        return EXIT_FAILURE;
    }
    int rootPid = getpid();

    pid_t pids[numOfChildren];
    for (int i = 0; i < numOfChildren; i++)
    {
        pids[i] = fork();
        if (pids[i] == -1)
        {
            cerr << "Failed to fork\n";
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0)
        {
            close(pipes[i][0]); //Close child read end

            int startIndex = i * recordLoadPerChild;
            int endIndex = (i == numOfChildren - 1) ? numOfRecords - 1 : ((i + 1) * recordLoadPerChild) - 1;
            int numOfRecords2 = endIndex + 1 - startIndex;
            if (DEBUG == 1)
            {
                cout << "NumOfRecords for process: " << i + 1 << " : " << numOfRecords2 << endl;
            }

            int recordLoadPerChild2 = numOfRecords2 / (numOfChildren - i);
            if (recordLoadPerChild2 == 0)
            {
                cerr << "Error, load should be at least 1 Record / sorter" << endl;
                return EXIT_FAILURE;
            }
            
            if (DEBUG == 1)
            {
                cout << "RecordLoadPerChild for process: " << i + 1 << " : " << recordLoadPerChild2 << endl;
            }

            int** pipes2 = new int*[numOfChildren - i]; // Dynamic array of pipes
            for (int j = 0; j < numOfChildren - i; j++)
            {
                pipes2[j] = new int[2]; // Each element is an array of two integers
                if (pipe(pipes2[j]) == -1)
                {
                    cerr << "Pipe Failed" << endl;
                    return 1;
                }
            }

            pid_t pids2[numOfChildren];
            for (int j = 0; j < numOfChildren - i; j++)
            {
                pids2[j] = fork();
                if (pids2[j] == -1)
                {
                    cerr << "Failed to fork\n";
                    exit(EXIT_FAILURE);
                }

                if (pids2[j] == 0)
                {
                    close(pipes2[j][0]); //Close child read end

                    int startIndex2 = startIndex + (j * recordLoadPerChild2);
                    int endIndex2 = (j == numOfChildren - i - 1) ?  startIndex + numOfRecords2 - 1 :  startIndex + ((j + 1) * recordLoadPerChild2) - 1;

                    char startIndexStr[10];
                    char endIndexStr[10];
                    sprintf(startIndexStr, "%d", startIndex2);
                    sprintf(endIndexStr, "%d", endIndex2);

                    char fdStr[10];
                    sprintf(fdStr, "%d", pipes2[j][1]);

                    char rpStr[10];
                    sprintf(rpStr, "%d", rootPid);

                    char* execArgs[] = { sorting1, dataFile, startIndexStr, endIndexStr, fdStr, rpStr, NULL };

                    if (j % 2 == 0)
                    {
                        execv(sorting1, execArgs);
                    }
                    else
                    {
                        execv(sorting2, execArgs);
                    }

                    cerr << "Execv failed\n";
                    exit(EXIT_FAILURE);
                }
            }

            char totalBuffer2[sizeof(Record) * numOfRecords2];
            char* buffers2[numOfChildren - i];
            char totalTimeBuffer2[sizeof(double) * 2 * numOfChildren - i];
            double* times2[numOfChildren - i];
            int buffers2Counts[numOfChildren - i];
            for (int j = 0; j < numOfChildren - i; j++)
            {
                close(pipes2[j][1]); // Close write end in parent

                int startIndex2 = startIndex + (j * recordLoadPerChild2);
                int endIndex2 = (j == numOfChildren - i - 1) ?  startIndex + numOfRecords2 - 1 : startIndex + ((j + 1) * recordLoadPerChild2) - 1;
                int numOfRecords3 = endIndex2 + 1 - startIndex2;

                buffers2[j] = new char[sizeof(Record) * numOfRecords3];
                buffers2Counts[j] = numOfRecords3;
                times2[j] = new double[2];
                readFromPipe(pipes2[j][0], buffers2[j], sizeof(Record) * numOfRecords3);
                readFromPipe(pipes2[j][0], times2[j], sizeof(double) * 2);
                if (DEBUG == 1)
                {
                    printf ("Child %d, sorter %d: Run time was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n", i, j, times2[j][0], times2[j][1]);
                }
                close(pipes2[j][0]); // Close read end in parent after reading
            }

            for (int j = 0; j < numOfChildren - i; j++)
            {
                int status2;
                waitpid(pids2[j], &status2, 0);  // Wait for each child process to terminate
            }

            MergeSortedArrays((Record*)totalBuffer2, (Record**)buffers2, buffers2Counts, numOfChildren - i);
            MergeTimeArrays((double*)totalTimeBuffer2, (double**)times2, numOfChildren - i);

            writeToPipe(pipes[i][1], totalBuffer2, sizeof(Record) * numOfRecords2);
            writeToPipe(pipes[i][1], totalTimeBuffer2, sizeof(double) * 2 * (numOfChildren - i));
            close(pipes[i][1]);

            for (int j = 0; j < numOfChildren - i; j++)
            {
                delete[] buffers2[j];
                delete[] times2[j];
                delete[] pipes2[j];
            }
            delete[] pipes2;

            for (int i = 0; i < numOfChildren; i++)
            {
                delete[] pipes[i];
            }
            delete[] pipes;
            delete[] records;

            kill(rootPid, SIGUSR1);
            return EXIT_SUCCESS;
        }
    }

    char totalBuffer[sizeof(Record) * numOfRecords];
    char* buffers[numOfChildren];
    double* times[numOfChildren];
    int buffersCounts[numOfChildren];
    for (int i = 0; i < numOfChildren; i++)
    {
        close(pipes[i][1]); // Close write end in parent

        int startIndex = i * recordLoadPerChild;
        int endIndex = (i == numOfChildren - 1) ? numOfRecords - 1 : ((i + 1) * recordLoadPerChild) - 1;
        int numOfRecords2 = endIndex + 1 - startIndex;

        buffers[i] = new char[sizeof(Record) * numOfRecords2];
        buffersCounts[i] = numOfRecords2;
        times[i] = new double[2 * (numOfChildren - i)];

        readFromPipe(pipes[i][0], buffers[i], sizeof(Record) * numOfRecords2);
        readFromPipe(pipes[i][0], times[i], sizeof(double) * 2 * (numOfChildren - i));
        close(pipes[i][0]); // Close read end in parent after reading
    }

    for (int i = 0; i < numOfChildren; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);  // Wait for each child process to terminate
    }

    MergeSortedArrays((Record*)totalBuffer, (Record**)buffers, buffersCounts, numOfChildren);

    Record* sortedRecords = (Record*)totalBuffer;
    writeString(1, "\n");
    for (int i = 0; i < numOfRecords; i++) 
    {
        int maxCustidLength = 12;

        writeString(1, sortedRecords[i].LastName);
        for (int j = strlen(sortedRecords[i].LastName); j < SIZEofBUFF; j++) 
        {
            writeString(1, " ");
        }

        writeString(1, sortedRecords[i].FirstName);
        for (int j = strlen(sortedRecords[i].FirstName); j < SIZEofBUFF; j++) 
        {
            writeString(1, " ");
        }

        // Calculate the number of digits in custid
        int custid = sortedRecords[i].custid;
        int custidLength = (custid == 0) ? 1 : (int)log10(custid) + 1;
        writeInt(1, custid);
        for (int j = custidLength; j < maxCustidLength; j++)
        {
            writeString(1, " ");
        }

        writeString(1, sortedRecords[i].postcode);
        for (int j = strlen(sortedRecords[i].postcode); j < SSizeofBUFF; j++)
        {
            writeString(1, " ");
        }

        writeString(1, "\n");
    }

    writeString(1, "\n");
    for (int i = 0; i < numOfChildren; i++)
    {
        for (int j = 0; j < 2 * (numOfChildren - i); j += 2)
        {
            writeString(1, "Child ");
            writeInt(1, i);
            writeString(1, ", sorter ");
            writeInt(1, j / 2);
            writeString(1, ": Run time was ");
            writeDouble(1, times[i][j]);
            writeString(1, " sec (REAL time) although we used the CPU for ");
            writeDouble(1, times[i][j + 1]);
            writeString(1, " sec (CPU time)\n");
        }
    }

    if (DEBUG == 1)
    {
        std::ofstream outFile("debug.txt");
        for (int i = 0; i < numOfRecords; i++)
        {
            outFile << sortedRecords[i].LastName << " " << sortedRecords[i].FirstName << " " << sortedRecords[i].custid << " " << sortedRecords[i].postcode << endl;
        }
        outFile.close();
    }

    for (int i = 0; i < numOfChildren; i++)
    {
        delete[] buffers[i];
        delete[] times[i];
        delete[] pipes[i];
    }
    delete[] pipes;
    delete[] records;

    return EXIT_SUCCESS;
}

int findMinIndex(Record** sourceArrays, int* indices, int* sourceArraysCounters, int numArrays)
{
    int minIndex = -1;
    int i;

    for (i = 0; i < numArrays; i++)
    {
        if (indices[i] < sourceArraysCounters[i]) // Check if this array still has elements
        {
            if (minIndex == -1 || RecordReader::CompareRecords(sourceArrays[i][indices[i]], sourceArrays[minIndex][indices[minIndex]]))
            {
                minIndex = i;
            }
        }
    }

    return minIndex;
}

void MergeSortedArrays(Record* destination, Record** sourceArrays, int* sourceArraysCounters, int numSourceArrays)
{
    int* indices = (int*)malloc(numSourceArrays * sizeof(int));
    int destinationIndex = 0;

    // Initialize indices for each source array
    for (int i = 0; i < numSourceArrays; i++)
    {
        if (DEBUG == 1)
        {
            printf("\n");
            printf("Sorted array %d\n", i);
            for (int j = 0 ; j < sourceArraysCounters[i]; j++)
            {
                printf("%s %s %d %s \n", sourceArrays[i][j].LastName, sourceArrays[i][j].FirstName, sourceArrays[i][j].custid, sourceArrays[i][j].postcode);
            }
            printf("\n");
        }

        indices[i] = 0;
    }

    while (true)
    {
        int minArrayIndex = findMinIndex(sourceArrays, indices, sourceArraysCounters, numSourceArrays);

        if (minArrayIndex == -1) // All arrays have been fully traversed
        {
            break;
        }

        // Copy the smallest element to the destination array
        destination[destinationIndex++] = sourceArrays[minArrayIndex][indices[minArrayIndex]];

        // Increment the index for the array from which the element was taken
        indices[minArrayIndex]++;
    }

    free(indices);
}

void MergeTimeArrays(double* destination, double** sourceArrays, int numArrays)
{
    int destinationIndex = 0;

    for (int i = 0; i < numArrays; i++)
    {
        destination[destinationIndex++] = sourceArrays[i][0];
        destination[destinationIndex++] = sourceArrays[i][1];
    }
}

void signalHandler(int sigNum)
{
    if (sigNum == SIGUSR1)
    {
        cout << "Splitter/Merger finished" << endl;
    }
    else if (sigNum == SIGUSR2)
    {
        cout << "Sorter finished" << endl;
    }
    else
    {
        cerr << "Received unknown signal" << endl;
    }
}

ssize_t readFromPipe(int fd, void *buf, size_t totalSize)
{
    size_t totalBytesRead = 0;

    while (true)
    {
        ssize_t bytesRead = read(fd, static_cast<char*>(buf) + totalBytesRead, totalSize - totalBytesRead);

        if (bytesRead == -1)
        {
            cerr << "Error reading from pipe: " << errno << endl;
            if (errno == EINTR || errno == 0)
            {
                continue; // Retry reading if interrupted
            }
            return -1;
        }

        totalBytesRead += bytesRead;

        // Check if all data is read
        if (totalBytesRead >= totalSize)
        {
            if (DEBUG)
            {
                cout << "Reading finished for pipe" << endl;
            }
            break;
        }
        else
        {
            if (DEBUG)
            {
                cout << "Read: " << totalBytesRead << " out of: " << totalSize << endl;
            }
        }
    }

    return totalBytesRead;
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

void writeString(int fd, const char* str)
{
    write(fd, str, strlen(str));
}

void writeInt(int fd, int value)
{
    char buffer[12];
    int i = 0;
    bool isNegative = false;

    if (value < 0)
    {
        isNegative = true;
        value = -value;
    }

    do
    {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    } while (value);

    if (isNegative)
    {
        buffer[i++] = '-';
    }

    for (int j = 0; j < i / 2; j++)
    {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    buffer[i] = '\0';
    writeString(fd, buffer);
}

void writeFractional(int fd, double fractional, int decimalPlaces)
{
    while (decimalPlaces-- > 0)
    {
        fractional *= 10;
        int digit = (int)fractional % 10;
        char digitChar = '0' + abs(digit);
        write(fd, &digitChar, 1);
    }
}

void writeDouble(int fd, double value)
{
    int integerPart = (int)value;
    double fractionalPart = value - (double)integerPart;

    if (value < 0 && integerPart == 0)
    {
        write(fd, "-", 1);
    }

    writeInt(fd, abs(integerPart));
    write(fd, ".", 1);
    writeFractional(fd, fractionalPart, 6); // 6 decimal places
}