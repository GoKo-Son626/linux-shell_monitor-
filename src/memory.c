#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/memory.h"

void get_memory_usage() {
    FILE *fp;
    char key[64];
    long value;
    long mem_total = 0, mem_free = 0, mem_buffers = 0, mem_cached = 0, mem_available = 0;

    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("无法读取/proc/meminfo");
        return;
    }

    while (fscanf(fp, "%s %ld", key, &value) != EOF) {
        if (strcmp(key, "MemTotal:") == 0) {
            mem_total = value;
        } else if (strcmp(key, "MemFree:") == 0) {
            mem_free = value;
        } else if (strcmp(key, "Buffers:") == 0) {
            mem_buffers = value;
        } else if (strcmp(key, "Cached:") == 0) {
            mem_cached = value;
        } else if (strcmp(key, "MemAvailable:") == 0) { // Prefer MemAvailable if present
            mem_available = value;
        }
        // Optimization: if we found all we need, break early.
        // If MemAvailable is found, we can often ignore MemFree, Buffers, Cached for "used" calculation.
        // However, for detailed display, it's good to get them all.
        // If MemAvailable is present, it's usually the most accurate indicator of truly available memory.
        if (mem_total && (mem_available || (mem_free && mem_buffers && mem_cached))) {
            // Found what we need, can break for efficiency
            // But we keep reading to ensure we get MemAvailable if it's there
            // and processed later than MemFree, Buffers, Cached.
        }
    }
    fclose(fp);

    double display_mem_used_kb;
    double display_mem_total_mb = mem_total / 1024.0;

    if (mem_available > 0) {
        // If MemAvailable is present, calculate used based on it
        // total - available is a good representation of what's *currently in use by applications + kernel*
        display_mem_used_kb = (double)(mem_total - mem_available);
    } else {
        // Fallback for older kernels without MemAvailable:
        // Consider free, buffers, and cached as "effectively free"
        display_mem_used_kb = (double)(mem_total - (mem_free + mem_buffers + mem_cached));
    }

    double mem_usage = 100.0 * display_mem_used_kb / mem_total;
    printf("内存使用率: %.2f%% (%.2fMB / %.2fMB)\n", mem_usage, display_mem_used_kb / 1024.0, display_mem_total_mb);
}
