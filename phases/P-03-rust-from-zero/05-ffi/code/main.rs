// main.rs -- fence verification: sizes, offsets, calls. Lesson docs/en.md.
use std::mem::{size_of, align_of};

#[repr(C)]
struct PcbFfi {
    pid: i32,
    state: i32,
    rsp: u64,
}

fn off_of_pid() -> usize {
    0
}

fn main() {
    println!("size={} align={} pid_off={}", size_of::<PcbFfi>(), align_of::<PcbFfi>(), off_of_pid());
    let p = PcbFfi { pid: 1, state: 2, rsp: 0xABCD };
    let base = &p as *const PcbFfi as usize;
    // Taking addresses needs no unsafe; dereferencing raw pointers does.
    let off_state = &p.state as *const i32 as usize - base;
    let off_rsp = &p.rsp as *const u64 as usize - base;
    // SAFETY: &p.pid is a live, aligned, in-bounds place of this stack value.
    let pid_via_raw = unsafe { *(&p.pid as *const i32) };
    println!("state_off={off_state} rsp_off={off_rsp} rsp_val={:#x} pid_raw={pid_via_raw}", p.rsp);
}

#[cfg(test)]
mod checks {
    use super::*;

    #[test]
    fn layout_matches_c() {
        assert_eq!(size_of::<PcbFfi>(), 16);
        assert_eq!(align_of::<PcbFfi>(), 8);
    }

    #[test]
    fn offsets_match_c() {
        let p = PcbFfi { pid: 1, state: 2, rsp: 0 };
        let base = &p as *const PcbFfi as usize;
        let so = &p.state as *const i32 as usize - base;
        let ro = &p.rsp as *const u64 as usize - base;
        assert_eq!((0, so, ro), (0, 4, 8));
    }

    #[test]
    fn add_vectors() {
        fn add(a: i64, b: i64) -> i64 {
            a.wrapping_add(b)
        }
        assert_eq!(add(40, 2), 42);
        assert_eq!(add(-5, 5), 0);
    }

    #[test]
    fn widths_never_wobble() {
        assert_eq!(size_of::<i32>(), 4);
        assert_eq!(size_of::<u64>(), 8);
    }
}
