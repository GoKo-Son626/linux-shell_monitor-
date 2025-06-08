#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/network.h"

// 用来存储上次的数据，做流量增量计算.
// 这是一个更复杂的场景，可能需要一个数据结构来存储每个接口的prev_rx_bytes和prev_tx_bytes
// 对于简单地显示总流量和总增量，我们可以累加所有接口的当前流量和之前的流量
static long long global_prev_rx_bytes = 0; // Global previous total received bytes
static long long global_prev_tx_bytes = 0; // Global previous total transmitted bytes

void get_network_usage() {
    FILE *fp;
    char buffer[256];
    char iface[16];
    long long current_rx_bytes = 0; // Total received bytes for this snapshot
    long long current_tx_bytes = 0; // Total transmitted bytes for this snapshot
    long long temp_rx_bytes, temp_tx_bytes;

    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL) {
        perror("无法读取 /proc/net/dev 文件");
        return;
    }

    // Skip the first two header lines
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);

    // Read data for each network interface
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Use a temporary variable for fields read from sscanf
        // to avoid overwriting current_rx_bytes/current_tx_bytes during parsing
        int fields = sscanf(buffer, "%s %lld %*d %*d %*d %*d %*d %*d %lld", iface, &temp_rx_bytes, &temp_tx_bytes);

        if (fields == 3) {
            // Exclude loopback interface
            if (strcmp(iface, "lo:") != 0) {
                // Accumulate current traffic for this snapshot
                current_rx_bytes += temp_rx_bytes;
                current_tx_bytes += temp_tx_bytes;

                // You were printing per interface, let's keep that for now but also
                // provide overall totals. If you only want totals, remove this section.
                printf("网络接口 %s:\n", iface);
                printf("  当前下载流量: %.2fMB\n", temp_rx_bytes / 1024.0 / 1024.0);
                printf("  当前上传流量: %.2fMB\n", temp_tx_bytes / 1024.0 / 1024.0);
            }
        }
    }
    fclose(fp);

    // Calculate total increments since the last call
    long long rx_diff_total = current_rx_bytes - global_prev_rx_bytes;
    long long tx_diff_total = current_tx_bytes - global_prev_tx_bytes;

    // Handle initial run where prev_bytes might be 0 or small, leading to negative diff
    if (global_prev_rx_bytes == 0 || rx_diff_total < 0) { // first run or counter reset
        rx_diff_total = 0;
    }
    if (global_prev_tx_bytes == 0 || tx_diff_total < 0) { // first run or counter reset
        tx_diff_total = 0;
    }

    // Print overall totals and increments
    printf("--- 整体网络流量 ---\n");
    printf("  总下载流量: %.2fMB (增量: %.2fMB)\n", current_rx_bytes / 1024.0 / 1024.0, rx_diff_total / 1024.0 / 1024.0);
    printf("  总上传流量: %.2fMB (增量: %.2fMB)\n", current_tx_bytes / 1024.0 / 1024.0, tx_diff_total / 1024.0 / 1024.0);

    // Update global previous bytes for the next call
    global_prev_rx_bytes = current_rx_bytes;
    global_prev_tx_bytes = current_tx_bytes;
}
