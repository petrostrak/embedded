# Week 1
## Concepts

- [ ] **`stdint.h` fixed-width types.** Use `uint32_t`, `int16_t`, etc. — never bare `int` — for anything hardware-facing.
- [ ] **Integer promotion and the usual arithmetic conversions.** Understand when operands get widened before an operation.
- [ ] **Signed overflow is undefined behaviour; unsigned wraps.** 
- [ ] **`sizeof`.** Know what it returns and that its type is `size_t`.
- [ ] **Alignment.** `_Alignof`, natural alignment per type.
- [ ] **Struct padding.** Predict a struct's size before checking it with `sizeof`.
- [ ] **The four toolchain stages as four separate commands:**
  - [ ] Preprocess (`gcc -E`)
  - [ ] Compile to assembly (`gcc -S`)
  - [ ] Assemble to object file (`gcc -c` / `as`)
  - [ ] Link (`gcc` / `ld`)
- [ ] **Sections.** Know what belongs in `.text`, `.rodata`, `.data`, `.bss` and why.

## Project — "Where does my variable live?"

### Setup

- [ ] Create `mem.c` with one of each storage case:
  - [ ] A global **initialised** variable
  - [ ] A global **uninitialised** variable
  - [ ] A `static` local
  - [ ] A `const` global
  - [ ] A string literal
  - [ ] A stack local

### Build and inspect

- [ ] Compile:
  ```
  gcc -c -Wall -Wextra -Werror -Wconversion -std=c11 mem.c -o mem.o
  ```
- [ ] `size -A mem.o` — per-section sizes
- [ ] `nm mem.o` — symbol table and section letters
- [ ] `objdump -h mem.o` — section headers
- [ ] `readelf -S mem.o` — section headers with flags

### Write it up

- [ ] One line per symbol: which section it landed in, and **why**.

| Symbol | Section | Why |
|---|---|---|
| global initialised | | |
| global uninitialised | | |
| `static` local | | |
| `const` global | | |
| string literal | | |
| stack local | | |

- [ ] Cross-check: does the stack local appear in `nm` output at all? Explain the result either way.

### The array experiment

- [ ] Add a 1000-element zero-initialised array to `mem.c`.
- [ ] Re-run `size -A mem.o` and record the before/after numbers.
- [ ] Explain why the object file did **not** grow by 4 kB.
- [ ] Follow-up: change the array to be initialised with non-zero values and re-run `size`. Explain the difference.

## Done when

- [ ] You can predict which section a declaration lands in before compiling it.
- [ ] You can explain the `.bss` size answer to someone else without notes.

---

# Integer promotion and the usual arithmetic conversions.
### Terminology

| Term | Plain meaning |
|---|---|
| **operand** | A value on one side of an operator. In `a + b`, both `a` and `b` are operands. |
| **rank** | A fixed order of the integer types. From highest to lowest: `long long`, `long`, `int`, `short`, `char`. Two types can be the same width but differ in rank. `int32_t` and `long` are both 32 bits wide, but `long` ranks higher. |
| **signed / unsigned** | A signed type can hold negative values. An unsigned type cannot. |
| **promote** | Convert a value to `int`, or to `unsigned int` if `int` cannot hold every value of the original type. |

### The seven steps

| Rule | What to do | Example |
|---|---|---|
| **1** | If either operand has a floating type, convert both operands to the wider floating type. Then stop. | `i32 / 2.0f` → both become `float`. Result: `float`. |
| **2** | Promote both operands. Do this to each operand on its own, before you compare them. | `u8 + u8` → each `uint8_t` becomes `int`. |
| **3** | If both operands now have the same type, stop. You are done. | `i32 + i32` → both are `int` already. Result: `int`. |
| **4** | If both operands are signed, or both are unsigned, convert the lower-ranked operand to the higher-ranked type. | `i32 + i64` → the `int` becomes `long long`. Result: `long long`. |
| **5** | If one operand is signed and one is unsigned, and the unsigned type ranks the same or higher, convert the signed operand to the unsigned type. | `u32 + i32` → equal rank, so the `int` becomes `unsigned int`. Result: `unsigned int`. Also `sz - i32`. |
| **6** | If the signed type ranks higher, and it can hold every value of the unsigned type, convert the unsigned operand to the signed type. | `u32 + i64` → `int64_t` holds every `uint32_t` value. Result: `long long`. |
| **7** | If the signed type ranks higher but cannot hold every value of the unsigned type, convert both operands to the unsigned version of the signed type. | `u32 + l` → `long` outranks `unsigned int`, but 32 bits cannot hold every `uint32_t` value. Result: `unsigned long`. |

