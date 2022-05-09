#include "Header.h"

static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;

void init(HANDLE hProcess) {
    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    lastCPU.HighPart = ftime.dwHighDateTime;
    lastCPU.LowPart = ftime.dwLowDateTime;

    GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser);
    lastSysCPU.HighPart = fsys.dwHighDateTime;
    lastSysCPU.LowPart = fsys.dwLowDateTime;
    lastUserCPU.HighPart = fuser.dwHighDateTime;
    lastUserCPU.LowPart = fuser.dwLowDateTime;
}

int main(int argc, char const *argv[]) {
    // iterate through all processes
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return 1;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        return 1;
    }
    do {
        // get process handle
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
        if (hProcess == NULL) {
            continue;
        }
        init(hProcess);
        Sleep(10);
        // get process CPU utilization
        double cpu = GetCPUUtilization(hProcess);
        // get process RAM utilization
        double ram = GetRAMUtilization(hProcess);
        // get process disk utilization
        double disk = GetDiskUtilization(hProcess);
        // get process network utilization
        double network = GetNetworkUtilization(hProcess);
        // print process name, CPU utilization, RAM utilization, disk utilization, network utilization
        printf("%-50s\t%5.1f\t%10.1f MB\t%10.1f MB/s\t%10.1f KB/s\n", pe32.szExeFile, cpu, ram, disk, network);
        // close process handle
        CloseHandle(hProcess);
    } while (Process32Next(hProcessSnap, &pe32));

    // close process snapshot handle
    CloseHandle(hProcessSnap);
    return 0;
}

double GetCPUUtilization(HANDLE hProcess) {
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;
    double percent;

    GetSystemTimeAsFileTime(&ftime);
    now.HighPart = ftime.dwHighDateTime;
    now.LowPart = ftime.dwLowDateTime;

    if (!GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser)) {
        printf("error %d\n", GetLastError());
        return 0;
    }
    sys.HighPart = fsys.dwHighDateTime;
    sys.LowPart = fsys.dwLowDateTime;
    user.HighPart = fuser.dwHighDateTime;
    user.LowPart = fuser.dwLowDateTime;

    percent = ((sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart));
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent /= numProcessors;

    return percent * 100;
}
double GetRAMUtilization(HANDLE hProcess) {
    // get process RAM utilization
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));
    return pmc.WorkingSetSize / 1024.0 / 1024.0;
}
double GetDiskUtilization(HANDLE hProcess) {
    IO_COUNTERS ioCounters;
    GetProcessIoCounters(hProcess, &ioCounters);
    return (ioCounters.ReadTransferCount + ioCounters.WriteTransferCount) / 1024.0 / 1024.0;
}
double GetNetworkUtilization(HANDLE hProcess) {
    return 0;
}