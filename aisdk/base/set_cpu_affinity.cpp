#include "set_cpu_affinity.h"

#include <cstdint>

namespace aisdk::base {

std::string SetThisThreadName(const std::string &name) {
    constexpr size_t kMaxAllowedLength = 15;
    char origin_name[kMaxAllowedLength + 1] = {0};
    pthread_getname_np(pthread_self(), origin_name, sizeof(origin_name));  // 获取线程名称
    const std::string final_name = (name.length() > kMaxAllowedLength ? name.substr(0, kMaxAllowedLength) : name);
    pthread_setname_np(pthread_self(), final_name.c_str());
    return std::string(origin_name, kMaxAllowedLength);
}

#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(USE_THREAD_AFFINITY))
void setCurrentThreadAffinityMask(int mask) {
    int err, syscallres;
    pid_t pid = (pid_t)syscall(__NR_gettid);
    syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
}
#else
void setCurrentThreadAffinityMask(int) {}
#endif

size_t get_sched_affinity() {
#ifdef __GLIBC__
    pid_t pid = syscall(SYS_gettid);
#else
#ifdef PI3
    pid_t pid = getpid();
#else
    pid_t pid = gettid();
#endif
#endif

    size_t ret = 0;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    int syscallret = syscall(__NR_sched_getaffinity, pid, sizeof(mask), &mask);
    if (syscallret) {
        fprintf(stderr, "get_sched_affinity syscall error %d\n", syscallret);
        // 实测会报错, mask被添加成任意值
        for (int i = 0; i < 8; i++) {
            if (CPU_ISSET(i, &mask)) {
                ret |= (1 << i);
            }
        }
        printf("CPU affinity mask = %d\n", ret);
        return 0;
    }

    return ret;
}

int set_sched_affinity(size_t thread_affinity_mask) {
#ifdef __GLIBC__
    pid_t pid = syscall(SYS_gettid);
#else
#ifdef PI3
    pid_t pid = getpid();
#else
    pid_t pid = gettid();
#endif
#endif
    int syscallret = syscall(__NR_sched_setaffinity, pid, sizeof(thread_affinity_mask), &thread_affinity_mask);
    if (syscallret) {
        fprintf(stderr, "syscall error %d\n", syscallret);
        return -1;
    }
    return 0;
}

int get_cpucount() {
    int count = 0;
    // get cpu count from /proc/cpuinfo
    FILE *fp = fopen("/proc/cpuinfo", "rb");
    if (!fp) return 1;

    char line[1024];
    while (!feof(fp)) {
        char *s = fgets(line, 1024, fp);
        if (!s) break;

        if (memcmp(line, "processor", 9) == 0) {
            count++;
        }
    }

    fclose(fp);

    if (count < 1) count = 1;

    if (count > (int)sizeof(size_t) * 8) {
        // clang-format off
        fprintf(stderr, "more than %d cpu detected, thread affinity may not work properly :(\n", (int)sizeof(size_t) * 8);
        // clang-format on
    }

    return count;
}

int get_max_freq_khz(int cpuid) {
    // first try, for all possible cpu
    char path[256];
    sprintf(path, "/sys/devices/system/cpu/cpufreq/stats/cpu%d/time_in_state", cpuid);
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        // second try, for online cpu
        sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/stats/time_in_state", cpuid);
        fp = fopen(path, "rb");
        if (fp) {
            int max_freq_khz = 0;
            while (!feof(fp)) {
                int freq_khz = 0;
                int nscan = fscanf(fp, "%d %*d", &freq_khz);
                if (nscan != 1) break;

                if (freq_khz > max_freq_khz) max_freq_khz = freq_khz;
            }
            fclose(fp);
            if (max_freq_khz != 0) return max_freq_khz;
            fp = NULL;
        }

        if (!fp) {
            // third try, for online cpu
            sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpuid);
            fp = fopen(path, "rb");

            if (!fp) return -1;

            int max_freq_khz = -1;
            auto ret = fscanf(fp, "%d", &max_freq_khz);
            (void)ret;

            fclose(fp);

            return max_freq_khz;
        }
    }

    int max_freq_khz = 0;
    while (!feof(fp)) {
        int freq_khz = 0;
        int nscan = fscanf(fp, "%d %*d", &freq_khz);
        if (nscan != 1) break;

        if (freq_khz > max_freq_khz) max_freq_khz = freq_khz;
    }
    fclose(fp);
    return max_freq_khz;
}

void swapSort(std::vector<int> &arr, std::vector<int> &idx, bool reverse) {
    if (reverse) {
        for (uint32_t i = 0; i < arr.size() - 1; ++i) {
            int maxVal = arr[i];
            uint32_t maxIdx = i;
            for (uint32_t j = i + 1; j < arr.size(); ++j) {
                if (arr[j] > maxVal) {
                    maxVal = arr[j];
                    maxIdx = j;
                }
            }
            // swap val
            int tmp = arr[maxIdx];
            arr[maxIdx] = arr[i];
            arr[i] = tmp;
            // swap idx
            tmp = idx[maxIdx];
            idx[maxIdx] = idx[i];
            idx[i] = tmp;
        }
    } else {
        for (uint32_t i = 0; i < arr.size() - 1; ++i) {
            int minVal = arr[i];
            uint32_t minIdx = i;
            for (uint32_t j = i + 1; j < arr.size(); ++j) {
                if (arr[j] < minVal) {
                    minVal = arr[j];
                    minIdx = j;
                }
            }
            // swap val
            int tmp = arr[minIdx];
            arr[minIdx] = arr[i];
            arr[i] = tmp;
            // swap idx
            tmp = idx[minIdx];
            idx[minIdx] = idx[i];
            idx[i] = tmp;
        }
    }
}
}  // namespace aisdk::base