# Traits card

- Promise: trait Name { fns }; keepers: impl Name for T { ... }.
- Consume: fn f<T: Bound>(t: &T) — bound = knowledge + gate.
- Generic fn: stamped per type (nm shows); trait object dyn: one door, runtime cost.
- Own heap: Box<T> (one), Vec<T> (growable, doubling). View: &[T]/&str.
- Print: impl Display for {} (fmt writes pieces). Debug {:?} via derive.
- Errors: E0277 = missing promise; read the bound it names.
