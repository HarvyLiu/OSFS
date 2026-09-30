// main.rs -- PCB table with exhaustive states. Lesson docs/en.md.
// Build: rustc --edition 2021 main.rs. Tests: rustc --edition 2021 --test main.rs.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum State {
    Unused,
    Runnable,
    Running,
    Zombie,
}

#[derive(Clone, Debug)]
struct Pcb {
    pid: i32,
    state: State,
    name: String,
}

struct Table {
    slots: [Pcb; 4],
}

impl Table {
    fn new() -> Self {
        let blank = || Pcb { pid: 0, state: State::Unused, name: String::new() };
        Table { slots: [blank(), blank(), blank(), blank()] }
    }

    fn alloc(&mut self, pid: i32, name: &str) -> Result<usize, &'static str> {
        for (i, s) in self.slots.iter_mut().enumerate() {
            if s.state == State::Unused {
                *s = Pcb { pid, state: State::Runnable, name: name.to_string() };
                return Ok(i);
            }
        }
        Err("table full")
    }

    fn find(&self, pid: i32) -> Option<usize> {
        self.slots
            .iter()
            .position(|s| s.state != State::Unused && s.pid == pid)
    }

    fn describe(&self, i: usize) -> &'static str {
        match self.slots[i].state {
            State::Unused => "free",
            State::Runnable => "ready",
            State::Running => "on-cpu",
            State::Zombie => "dead-awaiting-reap",
        }
    }
}

fn main() {
    let mut t = Table::new();
    let a = t.alloc(1, "init").expect("slot for init");
    let b = t.alloc(2, "shell").expect("slot for shell");
    t.slots[a].state = State::Running;
    t.slots[b].state = State::Zombie;
    println!("{}({}) is {}, {}({}) is {}", t.slots[a].pid, t.slots[a].name, t.describe(a), t.slots[b].pid, t.slots[b].name, t.describe(b));
    let f = t.find(2).expect("shell present");
    t.slots[f].state = State::Unused;
    println!("reaped pid 2; find now: {:?}", t.find(2));
    let fill = t
        .alloc(3, "x")
        .and(t.alloc(4, "y"))
        .and(t.alloc(5, "z"))
        .and(t.alloc(6, "w"))
        .and(t.alloc(7, "v"));
    println!("alloc 5 more: {fill:?}");
}

#[cfg(test)]
mod checks {
    use super::*;

    #[test]
    fn lifecycle_runs() {
        let mut t = Table::new();
        let a = t.alloc(1, "init").unwrap();
        let b = t.alloc(2, "shell").unwrap();
        t.slots[a].state = State::Running;
        assert_eq!(t.describe(a), "on-cpu");
        assert_eq!(t.find(2), Some(b));
        t.slots[b].state = State::Unused;
        assert_eq!(t.find(2), None);
    }

    #[test]
    fn full_table_errors() {
        let mut t = Table::new();
        for i in 0..4 {
            assert!(t.alloc(i, "x").is_ok());
        }
        assert_eq!(t.alloc(99, "y"), Err("table full"));
    }

    #[test]
    fn describe_covers_all_states() {
        let mut t = Table::new();
        let i = t.alloc(1, "a").unwrap();
        t.slots[i].state = State::Unused;
        assert_eq!(t.describe(i), "free");
        t.slots[i].state = State::Runnable;
        assert_eq!(t.describe(i), "ready");
        t.slots[i].state = State::Running;
        assert_eq!(t.describe(i), "on-cpu");
        t.slots[i].state = State::Zombie;
        assert_eq!(t.describe(i), "dead-awaiting-reap");
    }

    #[test]
    fn missing_lookup_is_none() {
        let t = Table::new();
        assert_eq!(t.find(42), None);
    }

    #[test]
    fn names_survive_moves() {
        let mut t = Table::new();
        let i = t.alloc(7, "daemon").unwrap();
        assert_eq!(t.slots[i].name, "daemon");
        assert_eq!(t.slots[i].pid, 7);
    }
}
