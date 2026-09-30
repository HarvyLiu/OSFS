// abi.rs -- repr(C) PCB half. Lesson docs/en.md.
#[repr(C)]
pub struct PcbFfi {
    pub pid: i32,
    pub state: i32,
    pub rsp: u64,
}

pub const PCB_STATE_RUNNING: i32 = 2;

#[no_mangle]
pub extern "C" fn kern_add(a: i64, b: i64) -> i64 {
    a.wrapping_add(b)
}
