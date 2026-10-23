// Buildbit v2.0 C FFI Demo (ffi_demo.bb)
// Demonstrates calling any external C function or OS library directly via `extern fn`!

extern fn puts(msg: str) -> i32
extern fn sinf(x: f32) -> f32
extern fn cosf(x: f32) -> f32

fn main() {
    puts("=== Buildbit v2.0 Zero-Overhead C FFI (extern fn) ===")
    puts("[C libc puts()] Called C standard library puts() directly from Buildbit!")

    let angle = 0.5235988 // pi / 6 (30 degrees)
    let s = sinf(angle)
    let c = cosf(angle)

    print("[C libm sinf(pi/6)] = ")
    println(s)
    print("[C libm cosf(pi/6)] = ")
    println(c)
}
