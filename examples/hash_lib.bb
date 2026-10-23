// Buildbit Standard Systems Library Module (hash_lib.bb)
// Demonstrates bitwise operators (&, |, ^, <<, >>), hex literals (0x...), and byte slices ([u8])

fn fnv1a_hash64(buf: [u8], len: i64) -> i64 {
    let hash = 0xCBF29CE484222325
    let prime = 0x100000001B3

    for i in 0..len {
        hash = (hash ^ buf[i]) * prime
    }
    // Mix upper and lower bits so result fits cleanly in positive 63-bit integer
    return (hash ^ (hash >> 32)) & 0x7FFFFFFFFFFFFFFF
}

fn bit_pack_rgba(r: i64, g: i64, b: i64, a: i64) -> i64 {
    return ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF)
}
