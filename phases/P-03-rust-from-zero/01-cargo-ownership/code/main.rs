// main.rs -- ownership verbs: build, borrow, mutate, view. Lesson docs/en.md.
// Build: rustc --edition 2021 main.rs. Tests: rustc --edition 2021 --test main.rs.
fn build_name(first: &str, last: &str) -> String {
    let mut s = String::from(first);
    s.push(' ');
    s.push_str(last);
    s
}

fn shout(s: &str) -> String {
    s.to_uppercase()
}

fn push_exclaim(s: &mut String) {
    s.push('!');
}

fn main() {
    let name = build_name("ada", "lovelace");
    println!("name={name}");
    let loud = shout(&name);
    println!("loud={loud} still-have={name}");
    let mut m = String::from("hi");
    push_exclaim(&mut m);
    println!("m={m}");
    let v = vec![10, 20, 30, 40];
    let mid = &v[1..3];
    println!("mid={mid:?} len={}", mid.len());
}

#[cfg(test)]
mod checks {
    use super::*;

    #[test]
    fn builds_full_name() {
        assert_eq!(build_name("ada", "lovelace"), "ada lovelace");
    }

    #[test]
    fn shout_borrows_and_returns_new() {
        let s = String::from("osfs");
        assert_eq!(shout(&s), "OSFS");
        assert_eq!(s, "osfs"); // input untouched: borrow, not move
    }

    #[test]
    fn exclaim_mutates_in_place() {
        let mut m = String::from("hi");
        push_exclaim(&mut m);
        assert_eq!(m, "hi!");
    }

    #[test]
    fn slice_views_without_copy() {
        let v = vec![10, 20, 30, 40];
        assert_eq!(&v[1..3], &[20, 30]);
    }
}
