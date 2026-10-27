CC = gcc
CFLAGS = -O3 -Wall -Wextra -Werror

all: bbc bench bench_freestanding hello_elf sim_c sim_bb_single sim_bb_multi sys_demo sys_demo_free ffi_demo os gpu_os gpu_desktop

bbc: bbc.c
	$(CC) $(CFLAGS) bbc.c -o bbc

gpu_os: bbc os/gpu_os.bb os/gpu_vfs.bb
	./bbc os/gpu_os.bb -lX11 -lm -o gpu_os

gpu_desktop: bbc os/gpu_desktop.bb os/gpu_vfs.bb os/gpu_mint_gui.h
	./bbc os/gpu_desktop.bb -lX11 -lm -o gpu_desktop

boot-mint-gpu: gpu_desktop
	./gpu_desktop

boot-gpu-os: gpu_os
	./gpu_os

os: bbc os/kernel.bb os/linux_abi.bb
	./bbc os/kernel.bb --kernel -o buildbit_os_kernel.elf

gui_kernel: bbc os/kernel_gui.bb os/linux_abi.bb os/baremetal_gui.h
	./bbc os/kernel_gui.bb --kernel -o buildbit_gui_kernel.elf

gui_iso: gui_kernel
	mkdir -p /tmp/bb_iso_root/boot/grub
	cp buildbit_gui_kernel.elf /tmp/bb_iso_root/boot/buildbit_gui_kernel.elf
	printf 'set timeout=0\nset default=0\nmenuentry "BuildbitOS v2.3.0 (Ring-0 1280x720 VBE Graphical Desktop)" {\n    multiboot /boot/buildbit_gui_kernel.elf\n    boot\n}\n' > /tmp/bb_iso_root/boot/grub/grub.cfg
	grub-mkrescue -o buildbit_gui_os.iso /tmp/bb_iso_root 2>/dev/null
	rm -rf /tmp/bb_iso_root

boot-gui-iso: gui_iso
	qemu-system-x86_64 -enable-kvm -cpu host -m 256M -vga std -cdrom buildbit_gui_os.iso -serial stdio -no-reboot

boot-os: os
	qemu-system-x86_64 -enable-kvm -cpu host -kernel buildbit_os_kernel.elf -m 128M -display none -serial stdio -no-reboot

compare-os: os os/linux_compare_init.c
	@mkdir -p /tmp/bb_linux_initramfs
	@$(CC) -O3 -msse2 -ffast-math -static os/linux_compare_init.c -lm -o /tmp/bb_linux_initramfs/init
	@(cd /tmp/bb_linux_initramfs && echo init | cpio -o -H newc --quiet | gzip -9 > /tmp/linux_bench_initramfs.cpio.gz)
	@echo "=== 1. BOOTING BUILDBIT-OS (30 KB ELF Kernel) ON QEMU/KVM ==="
	@t0=$$(date +%s%N); \
	qemu-system-x86_64 -enable-kvm -cpu host -kernel buildbit_os_kernel.elf -m 128M -display none -serial stdio -no-reboot; \
	t1=$$(date +%s%N); \
	echo ">>> BuildbitOS Total Cold-Boot-to-Poweroff Wall Time: $$(( (t1 - t0) / 1000000 )) ms"
	@echo ""
	@echo "=== 2. BOOTING LINUX 6.14 (vmlinuz-6.14.0-37-generic + C Initramfs) ON QEMU/KVM ==="
	@t2=$$(date +%s%N); \
	qemu-system-x86_64 -enable-kvm -cpu host -kernel /boot/vmlinuz-6.14.0-37-generic -initrd /tmp/linux_bench_initramfs.cpio.gz -append "console=ttyS0 quiet panic=-1" -m 128M -display none -serial stdio -no-reboot; \
	t3=$$(date +%s%N); \
	echo ">>> Linux 6.14 Total Cold-Boot-to-Poweroff Wall Time: $$(( (t3 - t2) / 1000000 )) ms"
	@rm -rf /tmp/bb_linux_initramfs /tmp/linux_bench_initramfs.cpio.gz

bench: bbc examples/bare_metal_bench.bb
	./bbc examples/bare_metal_bench.bb -o bench

bench_freestanding: bbc examples/bare_metal_bench.bb
	./bbc examples/bare_metal_bench.bb --freestanding -o bench_freestanding

hello_elf: bbc examples/hello_elf.bb
	./bbc examples/hello_elf.bb --native-elf -o hello_elf

sim_c: examples/nbody_ray_sim.c
	$(CC) -O3 -march=native -mtune=native -ffast-math examples/nbody_ray_sim.c -o sim_c

sim_bb_single: bbc examples/nbody_ray_sim.bb
	./bbc examples/nbody_ray_sim.bb --freestanding -o sim_bb_single

sim_bb_multi: bbc examples/nbody_ray_sim.bb
	./bbc examples/nbody_ray_sim.bb -o sim_bb_multi

sys_demo: bbc examples/systems_demo.bb examples/hash_lib.bb
	./bbc examples/systems_demo.bb -o sys_demo

sys_demo_free: bbc examples/systems_demo.bb examples/hash_lib.bb
	./bbc examples/systems_demo.bb --freestanding -o sys_demo_free

ffi_demo: bbc examples/ffi_demo.bb
	./bbc examples/ffi_demo.bb -lm -o ffi_demo

compare: sim_c sim_bb_single sim_bb_multi
	@echo "=================================================================="
	@echo "1. Standard Idiomatic C (gcc -O3 -march=native -ffast-math)"
	@echo "=================================================================="
	./sim_c
	@echo ""
	@echo "=================================================================="
	@echo "2. Buildbit Single-Core Freestanding (--freestanding, 1 Core SoA)"
	@echo "=================================================================="
	./sim_bb_single
	@echo ""
	@echo "=================================================================="
	@echo "3. Buildbit Default Multi-Core + Auto-SoA + Temporal Loop Fusion"
	@echo "=================================================================="
	./sim_bb_multi

run: all
	@echo "--- 1. Multi-Core + AVX-512/NEON + Direct AMDGPU Zero-Copy GTT/VRAM Mode ---"
	./bench
	@echo ""
	@echo "--- 2. Freestanding Zero-Libc Raw Hardware Syscall Mode ---"
	./bench_freestanding
	@echo ""
	@echo "--- 3. Systems Programming Demo (Imports, Bitwise, Syscalls, File I/O) ---"
	./sys_demo
	@rm -f buildbit_test.bin
	@echo ""
	@echo "--- 4. Zero-Overhead C FFI Demo (extern fn) ---"
	./ffi_demo
	@echo ""
	@echo "--- 5. Direct x86_64 ELF64 Machine-Code Emitter Mode (Zero GCC/LD) ---"
	./hello_elf

compare-swarm: bbc examples/ecs_swarm_bench.c examples/ecs_swarm_bench.bb
	$(CC) -std=c11 -O2 examples/ecs_swarm_bench.c -o swarm_c_o2 -lm
	$(CC) -std=c11 -O3 -march=native -ffast-math examples/ecs_swarm_bench.c -o swarm_c_o3 -lm
	./bbc examples/ecs_swarm_bench.bb --freestanding -o swarm_bb_1core
	./bbc examples/ecs_swarm_bench.bb -o swarm_bb_full
	@echo "=== 1. Standard C11 AoS (gcc -std=c11 -O2) ==="
	./swarm_c_o2
	@echo ""
	@echo "=== 2. Max GCC C11 AoS (gcc -std=c11 -O3 -march=native -ffast-math) ==="
	./swarm_c_o3
	@echo ""
	@echo "=== 3. Buildbit 1-Core (--freestanding SoA + SROA-16 SIMD Register Pinning) ==="
	./swarm_bb_1core
	@echo ""
	@echo "=== 4. Buildbit Default (Multi-Core + SoA + SROA-16 SIMD + AMDGPU gpu_arena) ==="
	./swarm_bb_full

compare-gpu-os: bbc examples/gpu_os_coupled_bench.c examples/gpu_os_coupled_bench.bb
	$(CC) -std=c11 -O2 examples/gpu_os_coupled_bench.c -o gpu_os_c_o2 -lm
	$(CC) -std=c11 -O3 -march=native -ffast-math examples/gpu_os_coupled_bench.c -o gpu_os_c_o3 -lm
	./bbc examples/gpu_os_coupled_bench.bb -o gpu_os_bb_cpu
	./bbc examples/gpu_os_coupled_bench.bb --gpu -o gpu_os_bb_gpu
	@echo "=== 1. Traditional Linux C11 AoS (gcc -std=c11 -O2) ==="
	./gpu_os_c_o2
	@echo ""
	@echo "=== 2. Max GCC Linux C11 AoS (gcc -std=c11 -O3 -march=native -ffast-math) ==="
	./gpu_os_c_o3
	@echo ""
	@echo "=== 3. Buildbit CPU (24-Thread OpenMP + AVX2 SIMD + SoA) ==="
	./gpu_os_bb_cpu
	@echo ""
	@echo "=== 4. Buildbit GPU-Resident OS (--gpu Radeon RX 9060 XT GDDR6 VRAM) ==="
	./gpu_os_bb_gpu

clean:
	rm -rf bbc bench bench_freestanding hello_elf sim_c sim_bb_single sim_bb_multi sys_demo sys_demo_free ffi_demo buildbit_os_kernel.elf buildbit_test.bin swarm_c_o2 swarm_c_o3 swarm_bb_1core swarm_bb_full gpu_os_c_o2 gpu_os_c_o3 gpu_os_bb_cpu gpu_os_bb_gpu build *.gen.c

