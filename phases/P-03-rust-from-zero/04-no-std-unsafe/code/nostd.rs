// nostd.rs -- core only. Lesson docs/en.md. No std, no main, own panic room.
#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic_room(info: &PanicInfo) -> ! {
    let _ = info;
    loop {
        core::hint::spin_loop();
    }
}

#[no_mangle]
pub extern "C" fn kern_add(a: i64, b: i64) -> i64 {
    a.wrapping_add(b)
}

/// Read one byte through a raw pointer. Caller promises validity.
pub unsafe fn peek(addr: *const u8) -> u8 {
    *addr
}
