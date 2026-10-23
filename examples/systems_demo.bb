// Buildbit v2.0 Systems Programming Demo (systems_demo.bb)
// Demonstrates:
//   1. Multi-file module import (`import "examples/hash_lib.bb"`)
//   2. Hex (`0x...`) & Binary (`0b...`) literals + Bitwise operators (`&`, `|`, `^`, `<<`, `>>`, `%`)
//   3. Direct Linux Kernel `syscall(39)` (`sys_getpid`)
//   4. Byte buffer allocation (`alloc(mem, u8, size)`)
//   5. Direct Kernel File I/O (`file_write` and `file_read`) + 64-bit FNV-1a Hash Verification
//   Works in BOTH standard mode and `--freestanding` zero-libc mode!

import "examples/hash_lib.bb"

fn main() {
    println("=== Buildbit v2.0 Systems-Complete Runtime ===")

    // 1. Direct Linux Kernel Syscall (#39 on x86_64 = sys_getpid)
    let pid = syscall(39)
    print("[Syscall #39] Process ID (PID) directly from kernel: ")
    println(pid)

    // 2. Bitwise RGBA packing & binary mask test
    let pixel = bit_pack_rgba(0xFF, 0x80, 0x40, 0b1111_1111)
    print("[Bitwise Ops] Packed 32-bit RGBA Pixel (0xFF8040FF) = ")
    println(pixel)

    // 3. Allocate a 64 KB byte buffer in an arena, fill with binary pattern, hash it
    arena sys_mem = 1_048_576
    let size = 65_536
    let tx_buf = alloc(sys_mem, u8, size)
    let rx_buf = alloc(sys_mem, u8, size)

    parallel for i in 0..size {
        tx_buf[i] = ((i * 73) ^ (i >> 3)) & 0xFF
    }

    let hash_before = fnv1a_hash64(tx_buf, size)
    print("[FNV-1a 64-bit Hash] Original 64 KB Buffer:  ")
    println(hash_before)

    // 4. Write binary buffer to disk via raw kernel syscall and read it back
    let written = file_write("buildbit_test.bin", tx_buf, size)
    let read_back = file_read("buildbit_test.bin", rx_buf, size)

    print("[Direct File I/O] Bytes written / read back: ")
    print(written)
    print(" / ")
    println(read_back)

    let hash_after = fnv1a_hash64(rx_buf, size)
    print("[FNV-1a 64-bit Hash] Read-Back 64 KB Buffer: ")
    println(hash_after)

    if hash_before == hash_after {
        println("[Integrity Check] PASSED! Disk payload matches 100% byte-for-byte.")
    } else {
        println("[Integrity Check] FAILED!")
    }

    reset(sys_mem)
}
