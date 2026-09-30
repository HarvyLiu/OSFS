// rust-pcb.rs -- State/Pcb/Table starter for Phase 10 Rust modules.
// Swap String for fixed [u8; 16] + no_std when freestanding. Lesson P-03/02.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum State {
    Unused,
    Runnable,
    Running,
    Zombie,
}

#[derive(Clone, Debug)]
pub struct Pcb {
    pub pid: i32,
    pub state: State,
    pub name: String,
}
