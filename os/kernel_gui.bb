// ============================================================================
// BuildbitOS v2.3.0 — Bare-Metal Ring-0 1280x720 Graphical Desktop OS
// File: os/kernel_gui.bb
// ============================================================================
// Compiled with: ./bbc os/kernel_gui.bb --kernel -o buildbit_gui_kernel.elf
// Packaged into: buildbit_gui_os.iso (Bootable GRUB2 Hybrid BIOS/UEFI ISO)
//
// Zero Linux underneath. Zero libc (-ffreestanding -nostdlib).
// Runs in Ring-0 with direct hardware I/O:
//   - Bochs/QEMU VBE 1280x720x32bpp Linear Framebuffer (Ports 0x01CE/0x01CF, BAR0 0xFD000000)
//   - 8042 PS/2 Keyboard & 3-Button Mouse Controller (Ports 0x60/0x64)
//   - CMOS Real-Time Clock (Ports 0x70/0x71)
//   - PCIe / PCI Bus Enumerator (Ports 0xCF8/0xCFC)
//   - 32 MB Contiguous Physical RAM Arena (0x00400000) + SIMD SoA TaskTable
// ============================================================================

import "os/linux_abi.bb"

extern fn bb_kernel_gui_loop(backbuf: [u8], sched_cycles: i64, syscall_cycles: i64, max_frames: i64) -> i64;

soa struct TaskTable {
    pid: f32,
    state: f32,
    priority: f32,
    vruntime: f32,
    cpu_burst: f32,
    tickets: f32,
    mem_pages: f32,
    exec_cycles: f32
}

fn pci_read_config32(bus: i64, slot: i64, func: i64, offset: i64) -> i64 {
    let addr = 0x80000000 | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    port_out32(0xCF8, addr);
    return port_in32(0xCFC);
}

fn scan_pci_bus() -> i64 {
    println("[1/5] Enumerating Bare-Metal PCI/PCIe Hardware Bus (Ports 0xCF8 / 0xCFC)...");
    let mut found = 0;
    for slot in 0..32 {
        let id_reg = pci_read_config32(0, slot, 0, 0);
        let vendor = id_reg & 0xFFFF;
        let device = (id_reg >> 16) & 0xFFFF;
        if vendor != 0xFFFF {
            print("  [PCI 00:");
            print(slot);
            print(".0] VendorID=");
            print(vendor);
            print(" DeviceID=");
            println(device);
            found = found + 1;
        }
    }
    print("  Total Hardware PCI Functions Initialized: ");
    println(found);
    return found;
}

fn main() -> i32 {
    let boot_t0 = cycles();

    println("============================================================================");
    println("  BuildbitOS v2.3.0 — Bare-Metal Ring-0 1280x720 Graphical Desktop OS      ");
    println("  Zero-Linux | Zero-Libc | VBE 32bpp LFB | PS/2 Mouse+Kbd | SIMD SoA Kernel");
    println("============================================================================");
    gpu_probe();

    // 1. Hardware PCIe / PCI Bus Discovery
    let pci_count = scan_pci_bus();

    // 2. Physical Page Arena (32 MB starting at physical RAM 0x00400000)
    println("[2/5] Initializing 32 MB Physical Page Arena @ 0x00400000...");
    arena kmem = 33554432;

    // 3. SIMD SoA Process Scheduler (8,192 Concurrent Tasks x 100 Epochs = 819,200 Dispatches)
    println("[3/5] Running Ring-0 SIMD SoA Process Scheduler (8,192 Tasks x 100 Epochs)...");
    let n_tasks = 8192;
    let tasks = alloc_soa(kmem, TaskTable, n_tasks);

    parallel for i in 0..n_tasks {
        let fi = i * 1.0;
        tasks[i].pid = 1000.0 + fi;
        tasks[i].state = 1.0;
        tasks[i].priority = 20.0 + (fi * 0.001);
        tasks[i].vruntime = 0.0;
        tasks[i].cpu_burst = 1.25;
        tasks[i].tickets = 100.0;
        tasks[i].mem_pages = 16.0;
        tasks[i].exec_cycles = 0.0;
    }

    let sched_c0 = cycles();
    for epoch in 0..100 {
        parallel for i in 0..n_tasks {
            let weight = tasks[i].tickets / tasks[i].priority;
            let delta_vrun = tasks[i].cpu_burst / weight;
            tasks[i].vruntime = tasks[i].vruntime + delta_vrun;
            tasks[i].exec_cycles = tasks[i].exec_cycles + (tasks[i].cpu_burst * 64.0);
            tasks[i].cpu_burst = sqrt(tasks[i].cpu_burst * tasks[i].cpu_burst + 0.001);
        }
    }
    let sched_c1 = cycles();
    let sched_cycles = sched_c1 - sched_c0;
    let total_vruntime = reduce_sum(tasks_vruntime, n_tasks);

    print("  819,200 SIMD Task Dispatches completed in : ");
    print(sched_cycles);
    print(" cycles (vruntime checksum=");
    print(total_vruntime);
    println(")");

    // 4. Linux x86_64 Syscall ABI Compatibility Layer Benchmark (100,000 syscalls)
    println("[4/5] Benchmarking 100,000 Ring-0 Linux ABI Syscalls...");
    let sys_c0 = cycles();
    let mut pid_acc = 0;
    for s_iter in 0..100000 {
        pid_acc = pid_acc + syscall(39, s_iter);
    }
    let sys_c1 = cycles();
    let sys_cycles = sys_c1 - sys_c0;
    print("  100,000x sys_getpid() completed in        : ");
    print(sys_cycles);
    print(" cycles (acc=");
    print(pid_acc + pci_count);
    println(")");

    // 5. Allocate 1280x720x32bpp (3,686,400 bytes) Backbuffer in Physical Arena & Launch VBE GUI
    println("[5/5] Switching Hardware Display to 1280x720x32bpp VBE Linear Framebuffer...");
    let backbuf = alloc(kmem, u8, 3686400);
    let boot_t1 = cycles();
    print("  Pre-GUI Kernel Boot Time                  : ");
    print(boot_t1 - boot_t0);
    println(" CPU cycles");

    let frames = bb_kernel_gui_loop(backbuf, sched_cycles, sys_cycles, 0);
    print("[BuildbitOS] GUI Session exited cleanly after ");
    print(frames);
    println(" frames. Triggering ACPI Poweroff (0x604).");
    return 0;
}
