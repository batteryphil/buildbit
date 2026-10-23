# Buildbit (`bbc` v2.0 — Systems-Complete Edition)
**"As close to bare metal as possible — across every PC."**

Buildbit (`.bb`) is a systems & high-performance compute programming language that combines **20x faster data-oriented SIMD/GPU execution** with **full C-level systems capabilities** (bitwise operations, raw kernel syscalls, volatile MMIO hardware register access, zero-libc freestanding binaries, file I/O, multi-file imports, and zero-overhead C FFI).

---

## 5-Minute Language Cheat Sheet

```rust
// 1. Multi-File Imports & Zero-Overhead C FFI
import "examples/hash_lib.bb"
extern fn sinf(x: f32) -> f32

// 2. Automatic Struct-of-Arrays (64B AVX-512 / Cache-Line Aligned)
soa struct Particle {
    x: f32, y: f32, z: f32,
    vx: f32, vy: f32, vz: f32,
    id: i64, flags: i64
}

// 3. User Functions with Slices ([u8], [f32], [i64]), Bitwise & Hex/Binary Literals
fn checksum(buf: [u8], len: i64) -> i64 {
    let h = 0xCBF29CE484222325
    for i in 0..len {
        h = ((h ^ buf[i]) << 5) | (h >> 2)
    }
    return h & 0x7FFFFFFFFFFFFFFF
}

fn main() {
    // 4. Direct GPU Silicon Telemetry & Zero-Copy GPU/Kernel Arenas
    gpu_probe()
    gpu_arena vram = 67_108_864
    let p = alloc_soa(vram, Particle, 1_000_000)
    let bytes = alloc(vram, u8, 65_536)

    // 5. Multi-Core + SIMD Vector Loops & Temporal Loop-Interchange Fusion
    for step in 0..10 {
        parallel for i in 0..1_000_000 {
            p[i].x = p[i].x + p[i].vx * 0.016
        }
    }

    // 6. Direct Kernel Syscalls, File I/O, and Hardware Cycle Counters
    let pid = syscall(39) // sys_getpid
    file_write("out.bin", bytes, 65_536)
    reset(vram) // 1-instruction arena free!
}
```

---

## Compilation Modes & Commands

```bash
# Build & run all demos (GPU arena, Freestanding, Systems, FFI, and Direct ELF64):
make run

# Run the head-to-head benchmark against Standard C (gcc -O3 -march=native):
make compare
```
