#include "Header.h"

void GetCPUUtilization(ProcessPerformance* p) {
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	double percent;

	GetSystemTimeAsFileTime(&ftime);
	now.HighPart = ftime.dwHighDateTime;
	now.LowPart = ftime.dwLowDateTime;

	if (!GetProcessTimes(p->hProcess, &ftime, &ftime, &fsys, &fuser)) {
		printf("error %d\n", GetLastError());
		return;
	}
	sys.HighPart = fsys.dwHighDateTime;
	sys.LowPart = fsys.dwLowDateTime;
	user.HighPart = fuser.dwHighDateTime;
	user.LowPart = fuser.dwLowDateTime;

	percent = ((sys.QuadPart - p->lastSysCPU.QuadPart) + (user.QuadPart - p->lastUserCPU.QuadPart));
	percent /= (now.QuadPart - p->lastCPU.QuadPart);
	percent /= numProcessors;

	p->CPU = percent * 100;

	p->lastCPU.HighPart = ftime.dwHighDateTime;
	p->lastCPU.LowPart = ftime.dwLowDateTime;

	p->lastSysCPU.HighPart = fsys.dwHighDateTime;
	p->lastSysCPU.LowPart = fsys.dwLowDateTime;
	p->lastUserCPU.HighPart = fuser.dwHighDateTime;
	p->lastUserCPU.LowPart = fuser.dwLowDateTime;

}
void GetRAMUtilization(ProcessPerformance* p) {
	// get process RAM utilization
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(p->hProcess, &pmc, sizeof(pmc));
	p->RAM = pmc.WorkingSetSize / 1024.0 / 1024.0;
}
void GetDiskUtilization(ProcessPerformance* p) {
	IO_COUNTERS ioCounters;
	GetProcessIoCounters(p->hProcess, &ioCounters);
	p->Disk = (ioCounters.ReadTransferCount + ioCounters.WriteTransferCount - p->lastIOCounter.ReadTransferCount - p->lastIOCounter.WriteTransferCount) / 1024.0 / 1024.0;
	p->lastIOCounter = ioCounters;
}
void GetNetworkUtilization(ProcessPerformance* p) {
	p->Network = 0;
}

ProcessPerformance *parseJson(char *data, int fileSize) {
	char *ptr = data;
	ProcessPerformance *p = NULL;
	int numerOfProcess = 0;

	numerOfProcess = countInstance(data);
	p = (ProcessPerformance *)calloc(numerOfProcess, sizeof(ProcessPerformance));

	if (p) {
		for (int i = 0; i < numerOfProcess; i++) {
			// get PID
			ptr = strstr(ptr, "\"process\"");
			if (!ptr) {
				break;
			}
			ptr = strchr(ptr, ':');
			if (!ptr) {
				break;
			}
			p[i].name = getNextString(data, ptr - data);

			// cpu
			ptr = strstr(ptr, "\"cpu\"");
			if (!ptr) {
				break;
			}

			ptr = strchr(ptr, ':');
			if (!ptr) {
				break;
			}
			ptr++;
			p[i].cpuThres = strtol(ptr, &ptr, 10);

			// memory
			ptr = strstr(ptr, "\"memory\"");
			if (!ptr) {
				break;
			}

			ptr = strchr(ptr, ':');
			if (!ptr) {
				break;
			}
			ptr++;
			p[i].ramThres = strtol(ptr, &ptr, 10);

			// disk
			ptr = strstr(ptr, "\"disk\"");
			if (!ptr) {
				break;
			}

			ptr = strchr(ptr, ':');
			if (!ptr) {
				break;
			}
			ptr++;
			p[i].diskThres = strtol(ptr, &ptr, 10);

			// network
			ptr = strstr(ptr, "\"network\"");
			if (!ptr) {
				break;
			}

			ptr = strchr(ptr, ':');
			if (!ptr) {
				break;
			}
			ptr++;
			p[i].networkThres = strtol(ptr, &ptr, 10);
		}
	}
	return p;
}

// count number of process from json list (count "process")
int countInstance(char *data) {
	int count = 0;
	char *ptr = data;

	do {
		ptr = strstr(ptr, "\"process\"");
		if (ptr) {
			count++;
			ptr++;
		}
	} while (ptr);
	return count;
}

char *getNextString(char *data, int start) {
	char *begin = NULL, *end = NULL, *res = NULL;

	begin = strchr(data + start, '"') + 1;
	if (!begin) {
		return NULL;
	}
	end = strchr(begin, '"');
	if (!end) {
		return NULL;
	}

	res = (char *)calloc(end - begin + 1, 1);
	if (res != 0) {
		strncpy_s(res, end - begin + 1, begin, end - begin);
	}
	return res;
}

void printInfo(ProcessPerformance *p, int size) {
	for (int i = 0; i < size; i++) {
		printf("Process: %s\n\tcpu: %d\n\tmemory: %d\n\tdisk: %d\n\tnetwork: %d\n", p[i].name, p[i].cpuThres, p[i].ramThres, p[i].diskThres, p[i].networkThres);
	}
}

void freeMemory(ProcessPerformance *p, int size) {
	for (int i = 0; i < size; i++) {
		free(p[i].name);
		CloseHandle(p[i].hProcess);
	}
	free(p);
}

bool readJson(char *fileName, void **output, unsigned long *size) {
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD fileSize = 0, byteRead = 0;
	char *data = NULL;

	hFile = CreateFile(fileName, GENERIC_ALL, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Cannot open file\n");
		return FALSE;
	}
	fileSize = GetFileSize(hFile, NULL);
	data = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fileSize);
	if (!ReadFile(hFile, data, fileSize, &byteRead, NULL)) {
		printf("Cannot open file\n");
		return FALSE;
	}
	*output = data;
	*size = fileSize;
	CloseHandle(hFile);
	return TRUE;
}

void WriteLog(char* logName, ProcessPerformance p) {
	FILE* f = NULL;
	time_t t;
	char* buffer = NULL;

	time(&t);
	buffer = (char*)malloc(1000);
	ctime_s(buffer, 1000, &t);
	buffer[strlen(buffer) - 1] = '\0';

	fopen_s(&f, logName, "a");

	if (p.CPU > p.cpuThres) {
		fprintf_s(f, "%s, PID: %d, Name: %s, CPU: %.2f\n", buffer, GetProcessId(p.hProcess), p.name, p.CPU);
	}
	else if (p.RAM > p.ramThres) {
		fprintf_s(f, "%s, PID: %d, Name: %s, RAM: %.2f\n", buffer, GetProcessId(p.hProcess), p.name, p.RAM);
	}
	else if (p.Disk > p.diskThres) {
		fprintf_s(f, "%s, PID: %d, Name: %s, Disk: %.2f\n", buffer, GetProcessId(p.hProcess), p.name, p.Disk);
	}
	else if (p.Network > p.networkThres) {
		fprintf_s(f, "%s, PID: %d, Name: %s, Network: %.2f\n", buffer, GetProcessId(p.hProcess), p.name, p.Network);
	}

	fclose(f);
}