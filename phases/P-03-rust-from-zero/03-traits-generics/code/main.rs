// main.rs -- traits, generics, Box, Display. Lesson docs/en.md.
// Build: rustc --edition 2021 main.rs. Tests: rustc --edition 2021 --test main.rs.
use std::fmt::Display;

trait Describe {
    fn describe(&self) -> String;
}

struct Pcb {
    pid: i32,
    name: String,
}

struct Job {
    id: i32,
    burst: i32,
}

impl Describe for Pcb {
    fn describe(&self) -> String {
        format!("pcb {} ({})", self.pid, self.name)
    }
}

impl Describe for Job {
    fn describe(&self) -> String {
        format!("job {} burst {}", self.id, self.burst)
    }
}

fn first<T>(s: &[T]) -> Option<&T> {
    if s.is_empty() {
        None
    } else {
        Some(&s[0])
    }
}

fn announce<T: Describe>(t: &T) {
    println!("announce: {}", t.describe());
}

fn main() {
    let p = Pcb { pid: 1, name: "init".to_string() };
    let j = Job { id: 7, burst: 3 };
    announce(&p);
    announce(&j);
    let v = vec![10, 20, 30];
    println!("first={:?} len={}", first(&v), v.len());
    let boxed: Box<Pcb> = Box::new(p);
    println!("boxed pid={}", boxed.pid);
    let nums: Vec<String> = Vec::new();
    println!("empty display: '{}' len={}", nums.len(), nums.len());
    print_with_display(&j);
}

fn print_with_display<T: Display>(t: &T) {
    println!("display: {t}");
}

impl Display for Job {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Job#{}(burst {})", self.id, self.burst)
    }
}

#[cfg(test)]
mod checks {
    use super::*;

    #[test]
    fn describes_both() {
        let p = Pcb { pid: 1, name: "init".to_string() };
        let j = Job { id: 7, burst: 3 };
        assert_eq!(p.describe(), "pcb 1 (init)");
        assert_eq!(j.describe(), "job 7 burst 3");
    }

    #[test]
    fn generic_first_two_types() {
        assert_eq!(first(&[10, 20]), Some(&10));
        assert_eq!(first::<char>(&[]), None);
    }

    #[test]
    fn box_moves_ownership() {
        let p = Pcb { pid: 9, name: "x".to_string() };
        let b = Box::new(p);
        assert_eq!(b.pid, 9);
    }

    #[test]
    fn display_formats() {
        let j = Job { id: 7, burst: 3 };
        assert_eq!(format!("{j}"), "Job#7(burst 3)");
    }

    #[test]
    fn empty_vec_first_is_none() {
        let v: Vec<i32> = Vec::new();
        assert_eq!(first(&v), None);
    }
}
