#include <stdio.h>
#include <Windows.h>
#include <Psapi.h>
#include <tlhelp32.h>
#include <time.h>

extern int numProcessors;

struct ProcessPerformance {
	char* name;
	double CPU, RAM, Disk, Network;
	int cpuThres, ramThres, diskThres, networkThres;
	HANDLE hProcess;
	ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
	IO_COUNTERS lastIOCounter;
};

void GetCPUUtilization(ProcessPerformance *p);
void GetRAMUtilization(ProcessPerformance *p);
void GetDiskUtilization(ProcessPerformance *p);
void GetNetworkUtilization(ProcessPerformance *p);
ProcessPerformance *parseJson(char *data, int fileSize);
int countInstance(char *data);
char *getNextString(char *data, int start);
bool readJson(char *fileName, void **output, unsigned long *size);
void printInfo(ProcessPerformance *p, int size);
void freeMemory(ProcessPerformance *p, int size);
void WriteLog(char* logName, ProcessPerformance p);