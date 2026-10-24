// ============================================================================
// Buildbit ECS Swarm & Task Scheduler Benchmark (examples/ecs_swarm_bench.bb)
// Demonstrates:
//   - Automatic Struct-of-Arrays (`soa struct`) 64B-aligned layout transposition
//   - Zero-Copy AMDGPU / Hugepage Arena (`gpu_arena`)
//   - Automatic Temporal Loop Fusion + 16-Lane SIMD Register Pinning (`SROA-16`)
// ============================================================================

soa struct TaskDescriptor {
    priority_weight: f32,
    cost_rate: f32,
    time_slice_us: f32,
    inhibitor_state: f32
}

soa struct SwarmEntity {
    pos_x: f32,
    pos_y: f32,
    vel_x: f32,
    vel_y: f32,
    drag_x: f32,
    drag_y: f32,
    stamina: f32,
    target_x: f32,
    target_y: f32
}

fn main() -> i32 {
    let n_entities = 262144;
    let n_epochs = 100;

    println("============================================================================");
    println("  Buildbit Benchmark (ECS Swarm Physics + Task Scheduler)");
    println("  Active Entities: 262144 | Epochs: 100 (26.2M Entity Updates)");
    println("============================================================================");

    gpu_arena pool = 33554432;
    let tasks = alloc_soa(pool, TaskDescriptor, n_entities);
    let entities = alloc_soa(pool, SwarmEntity, n_entities);

    parallel for i in 0..n_entities {
        let fi = (i * 1.0) * 0.0001;
        tasks[i].priority_weight = 0.25 + (fi * 0.001);
        tasks[i].cost_rate = 0.05;
        tasks[i].time_slice_us = 0.0;
        tasks[i].inhibitor_state = 0.10;

        entities[i].pos_x = fi;
        entities[i].pos_y = 0.0 - fi;
        entities[i].vel_x = 0.01;
        entities[i].vel_y = -0.01;
        entities[i].drag_x = 0.05;
        entities[i].drag_y = 0.10;
        entities[i].stamina = 0.80;
        entities[i].target_x = 10.0;
        entities[i].target_y = 10.0;
    }

    let t0 = wall_ms();
    let c0 = cycles();

    for epoch in 0..n_epochs {
        parallel for i in 0..n_entities {
            let inh = tasks[i].inhibitor_state * 0.80 + tasks[i].priority_weight * 0.25 + 0.01;
            tasks[i].inhibitor_state = inh;
            tasks[i].time_slice_us = (16666.0 * tasks[i].priority_weight) / (1.0 + inh);

            let dx = entities[i].target_x - entities[i].pos_x;
            let dy = entities[i].target_y - entities[i].pos_y;
            let dist = sqrt(dx * dx + dy * dy + 0.001);
            let fx = (dx / dist) * 0.5 - entities[i].drag_x * 0.2;
            let fy = (dy / dist) * 0.5 - entities[i].drag_y * 0.2;

            entities[i].vel_x = (entities[i].vel_x + fx * 0.016) * 0.95;
            entities[i].vel_y = (entities[i].vel_y + fy * 0.016) * 0.95;
            entities[i].pos_x = entities[i].pos_x + entities[i].vel_x * 0.016;
            entities[i].pos_y = entities[i].pos_y + entities[i].vel_y * 0.016;

            entities[i].stamina = entities[i].stamina * 0.999 - tasks[i].cost_rate * 0.001;
            tasks[i].priority_weight = tasks[i].priority_weight * 0.98 + (1.0 - entities[i].stamina) * 0.02;
        }
    }

    let c1 = cycles();
    let t1 = wall_ms();

    let sum_q = reduce_sum(tasks_time_slice_us, n_entities);
    let sum_x = reduce_sum(entities_pos_x, n_entities);
    let sum_e = reduce_sum(entities_stamina, n_entities);
    let checksum = sum_q + sum_x + sum_e;

    print("  Wall Time      : ");
    print(t1 - t0);
    println(" ms");
    print("  CPU Cycles     : ");
    print(c1 - c0);
    println(" cycles");
    print("  Checksum       : ");
    println(checksum);

    return 0;
}
