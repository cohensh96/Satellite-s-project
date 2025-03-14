#include "ResourceMonitor.h"
#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <cerrno>
#include <cstring>

void ResourceMonitor::PrintCurrentUsage() {
    struct rusage usage={};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        std::cout << "Current usage data:" << std::endl;
        std::cout << "CPU time in user mode:"
                  << usage.ru_utime.tv_sec << "Seconds"
                  << usage.ru_utime.tv_usec << "Microseconds" << std::endl;
        std::cout << " CPU time in system mode:"
                  << usage.ru_stime.tv_sec << "Seconds"
                  << usage.ru_stime.tv_usec << "Microseconds" << std::endl;
        std::cout << "Maximum memory used (ru_maxrss):"
                  << usage.ru_maxrss << "Kilobytes" << std::endl;
    } else {
        std::cerr << "Error in getrusage:" << strerror(errno) << std::endl;
    }
}

void ResourceMonitor::PrintResourceLimits() {
    struct rlimit limit={};
    std::cout << "Resource Limits: " << std::endl;
    // Virtual address limit check (RLIMIT_AS)
    if (getrlimit(RLIMIT_AS, &limit) == 0) {
        std::cout << "Virtual memory limit (RLIMIT_AS):" << std::endl;
        std::cout << "Soft limit: " << limit.rlim_cur << "Bytes" << std::endl;
        std::cout << "Hard limit: " << limit.rlim_max << "Bytes" << std::endl;
    } else {
        std::cerr << "Error in getrlimit(RLIMIT_AS):" << strerror(errno) << std::endl;
    }
    // Checking the data memory limit (RLIMIT_DATA)
    if (getrlimit(RLIMIT_DATA, &limit) == 0) {
        std::cout << "Data memory limit (RLIMIT_DATA): " << std::endl;
        std::cout << "Soft limit: " << limit.rlim_cur << " Bytes" << std::endl;
        std::cout << "Hard limit: " << limit.rlim_max << " Bytes" << std::endl;
    } else {
        std::cerr << "Error in getrlimit(RLIMIT_DATA):" << strerror(errno) << std::endl;
    }
}

void ResourceMonitor::PrintUsageDiff(const struct rusage& before, const struct rusage& after) {
    long userSecDiff = after.ru_utime.tv_sec - before.ru_utime.tv_sec;
    long userUSecDiff = after.ru_utime.tv_usec - before.ru_utime.tv_usec;
    long sysSecDiff  = after.ru_stime.tv_sec - before.ru_stime.tv_sec;
    long sysUSecDiff  = after.ru_stime.tv_usec - before.ru_stime.tv_usec;

    std::cout << "The difference in resource use: " << std::endl;
    std::cout << "CPU time in user mode: " << userSecDiff << " Seconds"
              << userUSecDiff << "Microseconds" << std::endl;
    std::cout << " CPU time in system mode:" << sysSecDiff << "Seconds"
              << sysUSecDiff << "Microseconds" << std::endl;
    std::cout << "Max Memory (ru_maxrss):"
              << after.ru_maxrss << "Kilobytes" << std::endl;
}
