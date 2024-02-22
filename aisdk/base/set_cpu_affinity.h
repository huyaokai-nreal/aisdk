#ifndef __SET_CPU_AFFINITY__
#define __SET_CPU_AFFINITY__

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <chrono>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

int get_cpucount();
int set_sched_affinity(size_t thread_affinity_mask);
int get_max_freq_khz(int cpuid);

void swapSort(std::vector<int> &arr, std::vector<int> &idx, bool reverse = true);

void SetThisThreadName(const std::string &name);

void setCurrentThreadAffinityMask(int);

#endif