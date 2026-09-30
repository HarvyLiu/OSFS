// main.rs -- safe wrappers over sharp edges + wrapping math. Lesson docs/en.md.
fn safe_peek_byte(slice: &[u8], i: usize) -> Option<u8> {
    if i < slice.len() {
        // SAFETY: i < len, so the pointer is in-bounds and alive (borrowed).
        Some(unsafe { *slice.as_ptr().add(i) })
    } else {
        None
    }
}

fn main() {
    let data = [10u8, 20, 30];
    println!("peek1={:?} peek9={:?}", safe_peek_byte(&data, 1), safe_peek_byte(&data, 9));
    let top = i64::MAX;
    println!("wrap={} plain-panics-in-debug", top.wrapping_add(1));
    println!("checked={:?}", top.checked_add(1));
}

#[cfg(test)]
mod checks {
    use super::*;

    #[test]
    fn peek_hits() {
        let d = [10u8, 20, 30];
        assert_eq!(safe_peek_byte(&d, 1), Some(20));
    }

    #[test]
    fn peek_misses() {
        let d = [10u8, 20, 30];
        assert_eq!(safe_peek_byte(&d, 9), None);
    }

    #[test]
    fn wrapping_is_modular() {
        assert_eq!(i64::MAX.wrapping_add(1), i64::MIN);
    }

    #[test]
    fn checked_reports() {
        assert_eq!(i64::MAX.checked_add(1), None);
        assert_eq!(1i64.checked_add(2), Some(3));
    }
}
