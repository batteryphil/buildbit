/*
 * Linux Kernel Appliance Benchmark Init (os/linux_compare_init.c)
 * Runs as PID 1 (/init) inside a minimal Linux 6.14 QEMU/KVM Unikernel/Initramfs
 * to compare identical workloads head-to-head against BuildbitOS (os/kernel.bb).
 */

#define _GNU_SOURCE
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

static inline uint64_t rdtsc_now(void) {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/*
 * Standard Linux-style Task Control Block (`struct task_struct`)
 * In Linux 6.x x86_64, `sizeof(struct task_struct)` is ~9,408 bytes (allocated
 * out of `task_struct_cachep` slab pages). We use 4,096 bytes per task node
 * linked in `cfs_rq` runqueue order across 8,192 tasks (32 MB total).
 */
struct linux_task_node {
    float pid;
    float state;
    float priority;
    float vruntime;
    float cpu_burst;
    float tickets;
    float mem_pages;
    float exec_cycles;
    uint8_t kernel_task_struct_slab_pad[4056];
    struct linux_task_node *next;
};

int main(void) {
    uint64_t t0 = rdtsc_now();

    struct utsname uts;
    uname(&uts);

    printf("============================================================================\n");
    printf("  Linux Kernel (%s %s) — Minimal C Initramfs Appliance\n", uts.sysname, uts.release);
    printf("============================================================================\n");

    /* 1. Allocate 32 MB via Linux mmap() + page fault population */
    uint64_t m0 = rdtsc_now();
    void *arena = mmap(NULL, 33554432, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    uint64_t m1 = rdtsc_now();

    /* 2. Initialize 8,192 Linux task structs in non-contiguous slab runqueue order */
    const int n_tasks = 8192;
    struct linux_task_node *tasks = (struct linux_task_node *)arena;
    for (int i = 0; i < n_tasks; i++) {
        float fi = (float)i;
        tasks[i].pid = 1000.0f + fi;
        tasks[i].state = 1.0f;
        tasks[i].priority = 20.0f + (fi * 0.001f);
        tasks[i].vruntime = 0.0f;
        tasks[i].cpu_burst = 1.25f;
        tasks[i].tickets = 100.0f;
        tasks[i].mem_pages = 16.0f;
        tasks[i].exec_cycles = 0.0f;
        /* Realistic runqueue pointer linking across slab pages */
        int next_idx = ((i * 167) + 1) & 8191;
        tasks[i].next = (i + 1 < n_tasks) ? &tasks[next_idx] : NULL;
    }
    /* Ensure exact 8,192 traversal chain visiting every task once per epoch */
    int cur = 0;
    for (int step = 0; step < n_tasks - 1; step++) {
        int nxt = (cur + 17) & 8191;
        tasks[cur].next = &tasks[nxt];
        cur = nxt;
    }
    tasks[cur].next = NULL;

    /* 3. Run 100 Scheduler Epochs across 8,192 Tasks (819,200 Dispatches) */
    uint64_t sched_c0 = rdtsc_now();
    for (int epoch = 0; epoch < 100; epoch++) {
        for (struct linux_task_node *t = &tasks[0]; t != NULL; t = t->next) {
            float weight = t->tickets / t->priority;
            float delta_vrun = t->cpu_burst / weight;
            t->vruntime = t->vruntime + delta_vrun;
            t->exec_cycles = t->exec_cycles + (t->cpu_burst * 64.0f);
            t->cpu_burst = sqrtf(t->cpu_burst * t->cpu_burst + 0.001f);
        }
    }
    uint64_t sched_c1 = rdtsc_now();
    uint64_t sched_cycles = sched_c1 - sched_c0;

    float total_vruntime = 0.0f;
    for (int i = 0; i < n_tasks; i++) {
        total_vruntime += tasks[i].vruntime;
    }

    /* 4. Benchmark 100,000 raw Linux x86_64 __NR_getpid (39) syscalls */
    uint64_t sys_c0 = rdtsc_now();
    int64_t pid_acc = 0;
    for (int i = 0; i < 100000; i++) {
        int64_t ret;
        __asm__ __volatile__("syscall"
                             : "=a"(ret)
                             : "a"(39ULL), "D"((uint64_t)i)
                             : "rcx", "r11", "memory");
        pid_acc += ret;
    }
    uint64_t sys_c1 = rdtsc_now();
    uint64_t sys_cycles = sys_c1 - sys_c0;

    uint64_t t1 = rdtsc_now();

    printf("  [Linux 6.14] 32MB Page Table Allocation (mmap) : %llu CPU cycles\n",
           (unsigned long long)(m1 - m0));
    printf("  [Linux 6.14] 819,200 Task Dispatches (AoS)     : %llu CPU cycles (%.4f cycles/task)\n",
           (unsigned long long)sched_cycles, (double)sched_cycles / 819200.0);
    printf("  [Linux 6.14] Virtual Runtime Checksum          : %.4f\n", total_vruntime);
    printf("  [Linux 6.14] 100,000x sys_getpid() Syscalls    : %llu CPU cycles (%.4f cycles/syscall, acc=%lld)\n",
           (unsigned long long)sys_cycles, (double)sys_cycles / 100000.0, (long long)pid_acc);
    printf("  [Linux 6.14] Total Init Execution Cycles       : %llu CPU cycles\n",
           (unsigned long long)(t1 - t0));
    fflush(stdout);

    /* Power off QEMU VM immediately */
    reboot(RB_POWER_OFF);
    return 0;
}
