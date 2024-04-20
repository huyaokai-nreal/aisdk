#pragma once
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string>
#include <vector>
namespace aisdk::base {
int get_cpucount();
size_t get_sched_affinity();
int set_sched_affinity(size_t thread_affinity_mask);
int get_max_freq_khz(int cpuid);

void swapSort(std::vector<int> &arr, std::vector<int> &idx, bool reverse = true);

std::string SetThisThreadName(const std::string &name);

void setCurrentThreadAffinityMask(int);
}  // namespace aisdk::base
