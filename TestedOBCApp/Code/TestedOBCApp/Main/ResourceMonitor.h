#ifndef RESOURCEMONITOR_H
#define RESOURCEMONITOR_H

#include <sys/resource.h>

// A department that provides tools for measuring performance and resources
class ResourceMonitor {
public:
    // Prints current usage data (CPU and memory times)
    static void PrintCurrentUsage();

    //Prints the limits that the system has set for the process (memory and data limit)
    static void PrintResourceLimits();

    // Calculates and prints the difference in usage data between two rusage readings
    static void PrintUsageDiff(const struct rusage& before, const struct rusage& after);
};

#endif //RESOURCEMONITOR_H
