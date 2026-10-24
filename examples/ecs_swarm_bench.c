/*
 * Generic C11 ECS Swarm & Task Scheduler Benchmark (examples/ecs_swarm_bench.c)
 * Compares idiomatic Array-of-Structs (AoS) C11 against Buildbit's automatic
 * Struct-of-Arrays (SoA) + 16-Lane SIMD Register Pinning (SROA-16).
 */

#define _GNU_SOURCE
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static inline int64_t rdtsc_cycles(void) {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (int64_t)(((uint64_t)hi << 32) | lo);
}

static inline double wall_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

typedef struct {
    uint32_t task_id;
    char     label[24];
    float    priority_weight;
    float    cost_rate;
    float    time_slice_us;
    float    inhibitor_state;
    uint64_t total_cycles;
    uint32_t ticks_executed;
    uint8_t  active;
} TaskDescriptor;

typedef struct {
    float pos_x, pos_y;
    float vel_x, vel_y;
    float mass;
    float hunger;
    float drag_y;
    float drag_x;
    float stamina;
    float memory;
    float target_x, target_y;
    uint8_t kind;
    uint8_t pad[15];
} SwarmEntity;

#define N_ENTITIES 262144
#define N_EPOCHS   100

int main(void) {
    printf("============================================================================\n");
    printf("  C11 AoS Reference Benchmark (ECS Swarm Physics + Task Scheduler)\n");
    printf("  Active Entities: %d | Epochs: %d (26.2M Entity Updates)\n",
           N_ENTITIES, N_EPOCHS);
    printf("============================================================================\n");

    TaskDescriptor *tasks = aligned_alloc(64, N_ENTITIES * sizeof(TaskDescriptor));
    SwarmEntity *entities = aligned_alloc(64, N_ENTITIES * sizeof(SwarmEntity));

    for (int i = 0; i < N_ENTITIES; i++) {
        float fi = (float)i * 0.0001f;
        tasks[i].task_id = (uint32_t)(i + 1);
        tasks[i].label[0] = 't'; tasks[i].label[1] = 's'; tasks[i].label[2] = 'k'; tasks[i].label[3] = '\0';
        tasks[i].priority_weight = 0.25f + (fi * 0.001f);
        tasks[i].cost_rate = 0.05f;
        tasks[i].time_slice_us = 0.0f;
        tasks[i].inhibitor_state = 0.10f;
        tasks[i].active = 1;

        entities[i].pos_x = fi;
        entities[i].pos_y = -fi;
        entities[i].vel_x = 0.01f;
        entities[i].vel_y = -0.01f;
        entities[i].hunger = 0.20f;
        entities[i].drag_y = 0.10f;
        entities[i].drag_x = 0.05f;
        entities[i].stamina = 0.80f;
        entities[i].memory = 0.0f;
        entities[i].target_x = 10.0f;
        entities[i].target_y = 10.0f;
    }

    double t0 = wall_ms();
    int64_t c0 = rdtsc_cycles();

    for (int epoch = 0; epoch < N_EPOCHS; epoch++) {
        for (int i = 0; i < N_ENTITIES; i++) {
            float inh = tasks[i].inhibitor_state * 0.80f + tasks[i].priority_weight * 0.25f + 0.01f;
            tasks[i].inhibitor_state = inh;
            tasks[i].time_slice_us = (16666.0f * tasks[i].priority_weight) / (1.0f + inh);

            float dx = entities[i].target_x - entities[i].pos_x;
            float dy = entities[i].target_y - entities[i].pos_y;
            float dist = sqrtf(dx * dx + dy * dy + 0.001f);
            float fx = (dx / dist) * 0.5f - entities[i].drag_x * 0.2f;
            float fy = (dy / dist) * 0.5f - entities[i].drag_y * 0.2f;

            entities[i].vel_x = (entities[i].vel_x + fx * 0.016f) * 0.95f;
            entities[i].vel_y = (entities[i].vel_y + fy * 0.016f) * 0.95f;
            entities[i].pos_x += entities[i].vel_x * 0.016f;
            entities[i].pos_y += entities[i].vel_y * 0.016f;

            entities[i].stamina = entities[i].stamina * 0.999f - tasks[i].cost_rate * 0.001f;
            tasks[i].priority_weight = tasks[i].priority_weight * 0.98f + (1.0f - entities[i].stamina) * 0.02f;
        }
    }

    int64_t c1 = rdtsc_cycles();
    double t1 = wall_ms();

    double checksum = 0.0;
    for (int i = 0; i < N_ENTITIES; i++) {
        checksum += tasks[i].time_slice_us + entities[i].pos_x + entities[i].stamina;
    }

    printf("  Wall Time      : %.4f ms\n", t1 - t0);
    printf("  CPU Cycles     : %lld cycles\n", (long long)(c1 - c0));
    printf("  Checksum       : %.4f\n", checksum);

    free(tasks);
    free(entities);
    return 0;
}
