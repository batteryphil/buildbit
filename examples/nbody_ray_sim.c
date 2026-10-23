/*
 * Standard Idiomatic C Benchmark (nbody_ray_sim.c)
 * Compiled with: gcc -O3 -march=native -mtune=native -ffast-math
 *
 * Workload:
 *   20 time-steps of a 3D Electromagnetic Lorentz Force Plasma Particle Pusher
 *   + Kinetic Energy Reduction over 4,000,000 entities (16 fields = 96 bytes/struct).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static inline int64_t rdtsc_cycles(void) {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (int64_t)(((uint64_t)hi << 32) | lo);
}

static inline float wall_ms(void) {
    static int64_t base_sec = 0;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (base_sec == 0) base_sec = ts.tv_sec;
    return (float)((double)(ts.tv_sec - base_sec) * 1000.0 + (double)ts.tv_nsec / 1e6);
}

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float ex, ey, ez;
    float charge_over_mass;
    int64_t id;
    int64_t cell_index;
    int64_t flags;
    int64_t generation;
    int64_t parent_id;
    int64_t material_tag;
} PlasmaEntity;

int main(void) {
    const int64_t n = 4000000;
    const int64_t steps = 20;
    const float dt = 0.005f;
    const float bz = 0.25f;

    printf("=== Standard Idiomatic C (gcc -O3 -march=native -ffast-math) ===\n");

    float total_t0 = wall_ms();
    PlasmaEntity *e = (PlasmaEntity *)malloc((size_t)n * sizeof(PlasmaEntity));
    float *energy = (float *)malloc((size_t)n * sizeof(float));

    for (int64_t i = 0; i < n; i++) {
        e[i].x = 1.0f;
        e[i].y = 2.0f;
        e[i].z = 3.0f;
        e[i].vx = 0.5f;
        e[i].vy = -0.25f;
        e[i].vz = 1.2f;
        e[i].ex = 0.1f;
        e[i].ey = 0.2f;
        e[i].ez = -0.05f;
        e[i].charge_over_mass = 1.5f;
        e[i].id = i;
        e[i].cell_index = 42;
        e[i].flags = 1;
        e[i].generation = 0;
        e[i].parent_id = -1;
        e[i].material_tag = 7;
    }

    float kernel_t0 = wall_ms();
    int64_t c_start = rdtsc_cycles();

    /* Run 20 simulation steps of 3D Lorentz Force (E + v x B) + Position Integration */
    for (int64_t step = 0; step < steps; step++) {
        for (int64_t i = 0; i < n; i++) {
            float qm = e[i].charge_over_mass * dt;
            float vx = e[i].vx + qm * (e[i].ex + e[i].vy * bz);
            float vy = e[i].vy + qm * (e[i].ey - vx * bz);
            float vz = e[i].vz + qm * e[i].ez;

            e[i].vx = vx;
            e[i].vy = vy;
            e[i].vz = vz;

            e[i].x = e[i].x + vx * dt;
            e[i].y = e[i].y + vy * dt;
            e[i].z = e[i].z + vz * dt;
        }
    }

    /* Compute final kinetic energy and sum */
    float total_energy = 0.0f;
    for (int64_t i = 0; i < n; i++) {
        energy[i] = 0.5f * (e[i].vx * e[i].vx + e[i].vy * e[i].vy + e[i].vz * e[i].vz);
        total_energy += energy[i];
    }

    int64_t c_end = rdtsc_cycles();
    float kernel_t1 = wall_ms();

    float sample_x = e[0].x;
    free(energy);
    free(e);
    float total_t1 = wall_ms();

    printf("Sample e[0].x after 20 steps:      %.4f\n", (double)sample_x);
    printf("Total Plasma Energy:               %.4f\n", (double)total_energy);
    printf("20-Step Kernel CPU Cycles:         %lld\n", (long long)(c_end - c_start));
    printf("20-Step Kernel Wall-Clock Time:    %.4f ms\n", (double)(kernel_t1 - kernel_t0));
    printf("End-to-End Total Time (w/ alloc):  %.4f ms\n", (double)(total_t1 - total_t0));
    return 0;
}
