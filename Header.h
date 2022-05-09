#include <stdio.h>
#include <Windows.h>
#include <Psapi.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <pdh.h>

double GetCPUUtilization(HANDLE hProcess);
double GetRAMUtilization(HANDLE hProcess);
double GetDiskUtilization(HANDLE hProcess);
double GetNetworkUtilization(HANDLE hProcess);