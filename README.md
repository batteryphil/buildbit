# Buildbit (`bbc` v2.0 — Systems & Bare-Metal OS Edition)
**"As close to bare metal as possible — across every PC."**

Buildbit (`.bb`) is a systems & high-performance compute programming language that combines **20x faster data-oriented SIMD/GPU execution** with **full C-level systems & Ring-0 OS capabilities** (`--kernel` bare-metal Multiboot OS mode, hardware x86 port I/O `port_in8`/`port_out8`/`port_in32`/`port_out32`, volatile MMIO `mmio_read32`/`mmio_write32`, raw kernel `syscall`, zero-libc `--freestanding` binaries, multi-file `import`, and zero-overhead C FFI `extern fn`).

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

    // 6. Direct Kernel Syscalls, Hardware Port I/O, MMIO, and Cycle Counters
    let pid = syscall(39) // sys_getpid
    file_write("out.bin", bytes, 65_536)
    reset(vram) // 1-instruction arena free!
}
```

---

## BuildbitOS: Bare-Metal Linux-Compatible Scratch OS (`os/kernel.bb`)

This repository includes **BuildbitOS** (`os/kernel.bb` & `os/linux_abi.bb`), a Ring-0 bare-metal operating system kernel written 100% in Buildbit and compiled via `bbc os/kernel.bb --kernel`:
- **Hardware Boot & Dual Console:** Multiboot1 ELF kernel with hardware FPU/SSE enable, COM1 115200-8N1 UART (`0x3F8`) + VGA Text Framebuffer (`0xB8000`).
- **PCI/PCIe Hardware Bus Discovery:** Scans all 32 slots x 8 functions directly via x86 hardware ports `0xCF8` / `0xCFC`.
- **O(1) Physical Page Arena:** 32 MB physical memory arena starting at `0x00400000` + direct volatile 32-bit MMIO verification.
- **SIMD `soa struct TaskTable` Process Scheduler:** Schedules 1,024 tasks across 100 epochs (102,400 task dispatches) in ~150 CPU cycles/dispatch using 64-byte aligned SIMD vectors.
- **Linux x86_64 Syscall ABI Compatibility Layer:** Dispatches standard Linux syscalls (`__NR_read`, `__NR_write`, `__NR_open`, `__NR_close`, `__NR_mmap`, `__NR_getpid`, `__NR_uname`, `__NR_sysinfo`, `__NR_getuid`) with an in-memory RamFS (`/proc/version`, `/etc/os-release`).

```bash
# Compile BuildbitOS and boot it on bare-metal virtual hardware in QEMU:
make boot-os

# Build & run all userland demos (GPU arena, Freestanding, Systems, FFI, and Direct ELF64):
make run

# Run the head-to-head benchmark against Standard C (gcc -O3 -march=native):
make compare
```
