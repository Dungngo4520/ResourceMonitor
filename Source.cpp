#include "Header.h"

int numProcessors = 0;

void init(HANDLE hProcess, ProcessPerformance* pp) {
	FILETIME ftime, fsys, fuser;

	pp->hProcess = hProcess;

	GetSystemTimeAsFileTime(&ftime);
	pp->lastCPU.HighPart = ftime.dwHighDateTime;
	pp->lastCPU.LowPart = ftime.dwLowDateTime;

	GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser);
	pp->lastSysCPU.HighPart = fsys.dwHighDateTime;
	pp->lastSysCPU.LowPart = fsys.dwLowDateTime;
	pp->lastUserCPU.HighPart = fuser.dwHighDateTime;
	pp->lastUserCPU.LowPart = fuser.dwLowDateTime;
	GetProcessIoCounters(hProcess, &pp->lastIOCounter);
}

int main(int argc, char const *argv[]) {
	// iterate through all processes
	SYSTEM_INFO sysInfo;
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	char* data = NULL;
	DWORD fileSize = 0;
	ProcessPerformance *p;
	int numberOfProcess = 0;

	if (!readJson("input.json", (void**)&data, &fileSize)) {
		printf("error reading file");
		return 1;
	}
	numberOfProcess = countInstance(data);
	p = parseJson(data, fileSize);

	//printInfo(p, countInstance(data));

	//get number of processor
	GetSystemInfo(&sysInfo);
	numProcessors = sysInfo.dwNumberOfProcessors;


	while (true) {
		// get snapshot of all processes
		hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == INVALID_HANDLE_VALUE) {
			return 1;
		}

		// init ProcessPerformance
		for (int i = 0; i < numberOfProcess; i++) {
			pe32.dwSize = sizeof(PROCESSENTRY32);
			if (!Process32First(hProcessSnap, &pe32)) {
				return 1;
			}
			do {
				if (strncmp(pe32.szExeFile, p[i].name, sizeof(p[i].name)) == 0) {
					HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
					if (hProcess == NULL) {
						continue;
					}
					init(hProcess, &p[i]);
					break;
				}
			} while (Process32Next(hProcessSnap, &pe32));
		}

		Sleep(1000);
		system("cls");

		// get performance
		for (int i = 0; i < numberOfProcess; i++) {
			DWORD exitCode = 0;
			GetExitCodeProcess(p[i].hProcess, &exitCode);
			if (exitCode == STILL_ACTIVE) {
				GetCPUUtilization(&p[i]);
				GetRAMUtilization(&p[i]);
				GetDiskUtilization(&p[i]);
				GetNetworkUtilization(&p[i]);

				if (p[i].CPU > p[i].cpuThres || p[i].RAM > p[i].ramThres || p[i].Disk > p[i].diskThres || p[i].Network > p[i].networkThres) {
					char* log = "log.txt";
					WriteLog(log, p[i]);
				}
				printf("%-50s\t%5.1f\t%10.1f MB\t%10.1f MB/s\t%10.1f KB/s\n", p[i].name, p[i].CPU, p[i].RAM, p[i].Disk, p[i].Network);
			}
			else {
				printf("%-50s\t%5s\t%10s MB\t%10s MB/s\t%10s KB/s\n", p[i].name, "unknown", "unknown", "unknown", "unknown");
			}
		}
	}

	CloseHandle(hProcessSnap);
	freeMemory(p, numberOfProcess);
	return 0;
}

