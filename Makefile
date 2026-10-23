CC = gcc
CFLAGS = -O3 -Wall -Wextra -Werror

all: bbc bench bench_freestanding hello_elf sim_c sim_bb_single sim_bb_multi sys_demo sys_demo_free ffi_demo os

bbc: bbc.c
	$(CC) $(CFLAGS) bbc.c -o bbc

os: bbc os/kernel.bb os/linux_abi.bb
	./bbc os/kernel.bb --kernel -o buildbit_os_kernel.elf

boot-os: os
	qemu-system-x86_64 -kernel buildbit_os_kernel.elf -m 128M -display none -serial stdio -no-reboot

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

clean:
	rm -f bbc bench bench_freestanding hello_elf sim_c sim_bb_single sim_bb_multi sys_demo sys_demo_free ffi_demo buildbit_os_kernel.elf buildbit_test.bin *.gen.c
