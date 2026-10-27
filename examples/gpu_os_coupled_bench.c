/* ============================================================================
 * Traditional Linux C11 Coupled 4-Subsystem OS Benchmark (AoS Layout)
 * ----------------------------------------------------------------------------
 * Workload: 4,194,304 Coupled OS Descriptors (134 MB AoS / 67 MB Hot State)
 *           x 50 Coupled Kernel Ticks = 419,430,400 Coupled Subsystem Ops
 *
 * Subsystems Coupled Every Tick (Exceeding CPU 32 MB L3 Cache):
 *   1. 100GbE Packet Firewall & Neighbor Flow Correlation ((i + 1) & mask)
 *   2. VFS Page-Cache Integrity & LRU Scrubbing (4.19M 4KB pages = 16 GB RAM)
 *   3. Batched io_uring Async Syscall Ring Completion
 *   4. Coupled Work-Stealing Fiber Scheduler (steals load from (i - 1) & mask)
 * ============================================================================ */
#define _GNU_SOURCE
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double wall_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

#define N_ITEMS 4194304
#define MASK    4194303
#define TICKS   50

typedef struct {
    uint32_t pid;
    char     comm[12];
    float    pkt_flow;
    float    vfs_page;
    float    sys_ring;
    float    task_vrun;
} LinuxOsDesc;

int main(void) {
    printf("=== [Linux C11 Coupled 4-Subsystem OS Benchmark (AoS Layout)] ===\n");
    printf("Descriptors: %d (67.1 MB Hot State, Exceeds CPU L3 Cache) | Ticks: %d\n",
           N_ITEMS, TICKS);

    LinuxOsDesc *sys = aligned_alloc(64, (size_t)N_ITEMS * sizeof(LinuxOsDesc));
    if (!sys) return 1;

    for (int i = 0; i < N_ITEMS; i++) {
        sys[i].pid       = (uint32_t)(i + 1);
        sys[i].comm[0]   = 'k';
        sys[i].pkt_flow  = 1.0f + (float)(i & 255) * 0.01f;
        sys[i].vfs_page  = 0.5f;
        sys[i].sys_ring  = 0.1f;
        sys[i].task_vrun = 0.0f;
    }

    double t0 = wall_ms();
    for (int t = 0; t < TICKS; t++) {
        /* Pass A: 100GbE Packet Firewall + VFS Page Scrubbing + io_uring Completion */
        for (int i = 0; i < N_ITEMS; i++) {
            int next_i = (i + 1) & MASK;
            float pf = sys[i].pkt_flow * 0.98f + sys[next_i].pkt_flow * 0.02f + 0.001f;
            float vp = sqrtf(sys[i].vfs_page * sys[i].vfs_page + pf * 0.05f + 0.0001f) * 0.99f;
            float sr = sys[i].sys_ring * 0.90f + (pf + vp) * 0.10f;
            sys[i].sys_ring = sr;
            sys[i].vfs_page = vp;
        }
        /* Pass B: Coupled Work-Stealing Fiber Scheduler (steals load from prev_i) */
        for (int i = 0; i < N_ITEMS; i++) {
            int prev_i = (i - 1) & MASK;
            float io_load = sys[i].sys_ring * 0.75f + sys[prev_i].sys_ring * 0.25f;
            float quantum = 16666.0f / (1.0f + io_load);
            sys[i].task_vrun = sys[i].task_vrun + quantum * 0.001f;
            sys[i].pkt_flow  = sys[i].pkt_flow * 0.95f + (sys[i].task_vrun * 0.0001f);
        }
    }
    double t1 = wall_ms();

    double sum = 0.0;
    for (int i = 0; i < N_ITEMS; i++) {
        sum += sys[i].task_vrun;
    }

    printf("Sample Descriptor[0] vrun : %.4f\n", sys[0].task_vrun);
    printf("Total Coupled vrun Sum    : %.2f\n", sum);
    printf("Elapsed Wall Time (ms)    : %.4f\n", t1 - t0);

    free(sys);
    return 0;
}
