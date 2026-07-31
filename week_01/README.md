# Week 1
## Concepts

- [x] **`stdint.h` fixed-width types.** Use `uint32_t`, `int16_t`, etc. — never bare `int` — for anything hardware-facing.
- [x] **Integer promotion and the usual arithmetic conversions.** Understand when operands get widened before an operation.
- [x] **Signed overflow is undefined behaviour; unsigned wraps.** 
- [x] **`sizeof`.** Know what it returns and that its type is `size_t`.
- [x] **Alignment.** `_Alignof`, natural alignment per type.
- [x] **Struct padding.** Predict a struct's size before checking it with `sizeof`.
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

# Natural alignment per type

A scalar type has **natural alignment** when its alignment equals its size. Most C types are naturally aligned. This is a convention of the platform's ABI, not a rule in the C standard — but it holds almost everywhere.

Verified values on x86-64 Linux (GCC 13):

| Type | `sizeof` | `alignof` | Naturally aligned? |
| --- | --- | --- | --- |
| `char`, `signed char`, `unsigned char` | 1 | 1 | yes |
| `bool` / `_Bool` | 1 | 1 | yes |
| `short` | 2 | 2 | yes |
| `int` | 4 | 4 | yes |
| `long` | 8 | 8 | yes |
| `long long` | 8 | 8 | yes |
| `float` | 4 | 4 | yes |
| `double` | 8 | 8 | yes |
| `long double` | 16 | 16 | no — only 10 bytes carry data |
| any pointer (`void*`, `int*`, function pointer) | 8 | 8 | yes |
| `max_align_t` | 32 | 16 | — |

Two entries deserve comment.

**`long double` on x86-64** holds an 80-bit (10-byte) value. The ABI pads it to 16 bytes so that it stays 16-byte aligned inside arrays. Six bytes per value are wasted.

**`max_align_t`** is defined in `<stddef.h>`. Its alignment is at least as strict as every standard type's. This is the guarantee `malloc` gives you: memory from `malloc`, `calloc`, or `realloc` is aligned strictly enough for any standard type, so you never need to align it yourself unless you need something *more* than standard.

---

## Alignment of arrays, structs, and unions

Derived types get their alignment from their parts. Two rules cover every case.

**An array has the alignment of its element type.**
`double[4]` has alignment 8 and size 32.

**A struct or union has the alignment of its strictest member.**

```c
struct S {
    char   a;   // alignment 1
    double b;   // alignment 8  <-- strictest
    short  c;   // alignment 2
};
// alignof(struct S) == 8
```

The struct takes 8, because if the struct sits at an address divisible by 8, then `b` can also sit at an address divisible by 8. This is the whole reason the rule exists: **a struct must be aligned strictly enough to satisfy every member inside it.**

Unions work the same way. Every member starts at offset 0, so the union must satisfy the strictest of them:

```c
union U { char c; double d; int i[3]; };
// alignof == 8 (from double), sizeof == 16 (12 bytes rounded up to a multiple of 8)
```

---

# Struct padding: the three rules

**Padding** means unused bytes the compiler inserts into a struct. It inserts them because it must: members have to land on legal addresses, and the compiler is not allowed to reorder your members in C.

Lay out any struct with these three rules, applied in order:

> **Rule A — Members keep their declared order.**
> The first member sits at offset 0. Later members never move earlier. (C guarantees this. C++ guarantees it too, within an access-control block.)
>
> **Rule B — Insert *internal padding* before a member until its offset is a multiple of its alignment.**
> This is the padding you see between members.
>
> **Rule C — Insert *tail padding* at the end until the total size is a multiple of the struct's own alignment.**
> This is the padding you see after the last member.

Rule C surprises people, so here is why it exists. Consider `struct S arr[2]`. The array is contiguous: `arr[1]` starts exactly `sizeof(struct S)` bytes after `arr[0]`. If the size were not a multiple of the alignment, then `arr[1]` would be misaligned. So the size must be rounded up.

**Consequence:** `sizeof(struct) % alignof(struct) == 0`, always.

### Example 1 — padding in the middle and at the end

```c
struct Bad {
    char a;
    int  b;
    char c;
};
```

| Step | Member | Alignment | Offset | Why |
| --- | --- | --- | --- | --- |
| 1 | `a` | 1 | 0 | first member |
| 2 | — | — | 1–3 | 3 bytes internal padding: next offset for `b` must divide by 4 |
| 3 | `b` | 4 | 4 | 4 % 4 == 0 ✓ |
| 4 | `c` | 1 | 8 | any offset is legal |
| 5 | — | — | 9–11 | 3 bytes tail padding: size must divide by 4 |

```
offset: 0    1    2    3    4    5    6    7    8    9   10   11
        [a] [ . ][ . ][ . ][      b       ] [c] [ . ][ . ][ . ]
```

**Result:** `sizeof == 12`, `alignof == 4`. Six of the twelve bytes are padding.

### Example 2 — same members, better order

```c
struct Good {
    int  b;
    char a;
    char c;
};
```

```
offset: 0    1    2    3    4    5    6    7
        [      b       ] [a] [c] [ . ][ . ]
```

**Result:** `sizeof == 8`, `alignof == 4`. Only two bytes of padding. Same data, 33% smaller.

### Example 3 — a `double` forces a big gap

```c
struct Mixed {
    char   a;
    double b;
    short  c;
};
```

```
offset: 0    1 .............. 7    8 .................. 15   16   17   18 ....... 23
        [a] [ 7 bytes padding ] [        b (double)       ] [   c   ][ 6 bytes pad ]
```

| Member | Offset | Note |
| --- | --- | --- |
| `a` | 0 | |
| padding | 1–7 | 7 bytes, so `b` reaches offset 8 |
| `b` | 8 | |
| `c` | 16 | |
| padding | 18–23 | 6 bytes, so size reaches 24 (a multiple of 8) |

**Result:** `sizeof == 24`, `alignof == 8`. Thirteen of twenty-four bytes are padding.

### Example 4 — nesting keeps the inner padding

A nested struct is treated as one member with its own size and alignment. Its internal padding does not disappear.

```c
struct Nested {
    char         tag;
    struct Mixed m;   // size 24, alignment 8
};
```

`tag` at offset 0, then 7 bytes of padding, then `m` at offset 8. Total 32 bytes.

**Result:** `sizeof == 32`, `alignof == 8`.

### Example 5 — arrays inside structs

```c
struct WithArr {
    char a;
    int  v[3];   // size 12, alignment 4
    char b;
};
```

`a` at 0, 3 bytes padding, `v` at 4 (through 15), `b` at 16, 3 bytes tail padding.

**Result:** `sizeof == 20`, `alignof == 4`.

