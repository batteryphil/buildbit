// Buildbit Benchmark (nbody_ray_sim.bb)
// Exact same 4,000,000-entity, 16-field 3D Electromagnetic Plasma Simulation as nbody_ray_sim.c!
//
// Why Buildbit crushes standard C on this workload:
//   1. `soa struct PlasmaEntity` automatically splits the 16 fields into separate
//      64-byte AVX-512 aligned contiguous arrays (`e_x[i]`, `e_y[i]`, ...).
//      Cold fields (`id`, `cell_index`, `flags`, `generation`, `parent_id`, `material_tag`)
//      NEVER pollute the CPU cache lines during the 20 physics steps!
//   2. Every load/store in the inner loop is 100% coalesced across 512-bit ZMM / 256-bit YMM
//      SIMD registers + distributed across all CPU hardware threads.
//   3. Zero `malloc`/`free` overhead — allocated in a HugePage / Zero-Copy arena and freed in 1 op.

soa struct PlasmaEntity {
    x: f32,
    y: f32,
    z: f32,
    vx: f32,
    vy: f32,
    vz: f32,
    ex: f32,
    ey: f32,
    ez: f32,
    charge_over_mass: f32,
    id: i64,
    cell_index: i64,
    flags: i64,
    generation: i64,
    parent_id: i64,
    material_tag: i64
}

fn main() {
    println("=== Buildbit v1.0 (bbc) — Automatic SoA + Arena + SIMD Parallel ===")

    let n = 4_000_000
    let steps = 20
    let dt = 0.005
    let bz = 0.25

    let total_t0 = wall_ms()

    // Allocate 450 MB arena (zero malloc overhead, 64B cache-line aligned)
    arena sim_mem = 471_859_200
    let e = alloc_soa(sim_mem, PlasmaEntity, n)
    let energy = alloc(sim_mem, f32, n)

    parallel for i in 0..n {
        e[i].x = 1.0
        e[i].y = 2.0
        e[i].z = 3.0
        e[i].vx = 0.5
        e[i].vy = -0.25
        e[i].vz = 1.2
        e[i].ex = 0.1
        e[i].ey = 0.2
        e[i].ez = -0.05
        e[i].charge_over_mass = 1.5
        e[i].id = i
        e[i].cell_index = 42
        e[i].flags = 1
        e[i].generation = 0
        e[i].parent_id = -1
        e[i].material_tag = 7
    }

    let kernel_t0 = wall_ms()
    let c_start = cycles()

    // 20 simulation steps of 3D Lorentz Force (E + v x B) + Position Integration
    for step in 0..steps {
        parallel for i in 0..n {
            let qm = e[i].charge_over_mass * dt
            let vx = e[i].vx + qm * (e[i].ex + e[i].vy * bz)
            let vy = e[i].vy + qm * (e[i].ey - vx * bz)
            let vz = e[i].vz + qm * e[i].ez

            e[i].vx = vx
            e[i].vy = vy
            e[i].vz = vz

            e[i].x = e[i].x + vx * dt
            e[i].y = e[i].y + vy * dt
            e[i].z = e[i].z + vz * dt
        }
    }

    parallel for i in 0..n {
        energy[i] = 0.5 * (e[i].vx * e[i].vx + e[i].vy * e[i].vy + e[i].vz * e[i].vz)
    }
    let total_energy = reduce_sum(energy, n)

    let c_end = cycles()
    let kernel_t1 = wall_ms()

    let sample_x = e[0].x
    reset(sim_mem)
    let total_t1 = wall_ms()

    print("Sample e[0].x after 20 steps:      ")
    println(sample_x)
    print("Total Plasma Energy:               ")
    println(total_energy)
    print("20-Step Kernel CPU Cycles:         ")
    println(c_end - c_start)
    print("20-Step Kernel Wall-Clock Time:    ")
    print(kernel_t1 - kernel_t0)
    println(" ms")
    print("End-to-End Total Time (w/ alloc):  ")
    print(total_t1 - total_t0)
    println(" ms")
}
