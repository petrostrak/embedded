# Week 1
## Concepts

- [x] **`stdint.h` fixed-width types.** Use `uint32_t`, `int16_t`, etc. — never bare `int` — for anything hardware-facing.
- [x] **[Integer promotion and the usual arithmetic conversions.](#integer-promotion-and-the-usual-arithmetic-conversions)** Understand when operands get widened before an operation.
- [x] **Signed overflow is undefined behaviour; unsigned wraps.** 
- [x] **`sizeof`.** Know what it returns and that its type is `size_t`.
- [x] **Alignment.** `_Alignof`, natural alignment per type.
- [x] **Struct padding.** Predict a struct's size before checking it with `sizeof`.
- [x] **The four toolchain stages as four separate commands:**
  - [x] Preprocess (`gcc -E`)
  - [x] Compile to assembly (`gcc -S`)
  - [x] Assemble to object file (`gcc -c` / `as`)
  - [x] Link (`gcc` / `ld`)
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

---

# The Four Stages of the GCC Toolchain

When you type `gcc hello.c -o hello`, it looks like one action. It's actually four programs running in sequence, each handing its output to the next:

```
hello.c → [preprocessor] → hello.i → [compiler] → hello.s → [assembler] → hello.o → [linker] → hello
 C source                expanded C            assembly            object file          executable
```

`gcc` is really a *driver* — a wrapper that decides which tools to run based on the file extension you give it. You can stop it at any stage and inspect the intermediate result.

## The running example

```c
// hello.c
#include <stdio.h>

#define GREETING "Hello, world!"
#define SQUARE(x) ((x) * (x))

int main(void) {
    printf("%s %d\n", GREETING, SQUARE(4));
    return 0;
}
```

## Stage 1 — Preprocess (`gcc -E`)

```bash
gcc -E hello.c -o hello.i
```

The preprocessor is a **text manipulator**. It doesn't understand C at all — it just does find-and-replace on the source before the real compiler sees it.

**What it does**

- Pastes the entire contents of `stdio.h` (and everything *that* includes) in place of the `#include` line
- Replaces `GREETING` with `"Hello, world!"` and `SQUARE(4)` with `((4) * (4))`
- Deletes all comments
- Resolves `#ifdef` / `#if` branches, keeping only the surviving code

The result is still valid C, just much bigger — typically 800+ lines for this tiny file:

```c
# 1 "hello.c"
# 1 "/usr/include/stdio.h" 1 3 4
extern int printf (const char *__restrict __format, ...);
...
# 5 "hello.c"
int main(void) {
    printf("%s %d\n", "Hello, world!", ((4) * (4)));
    return 0;
}
```

Those `# 5 "hello.c"` line markers are how error messages later can still point back at your original file and line number.

**Why you'd run this manually:** debugging a macro that isn't expanding the way you expect, or finding out which header is actually being picked up.

## Stage 2 — Compile to assembly (`gcc -S`)

```bash
gcc -S hello.i -o hello.s
```

This is the real compiler (`cc1` under the hood). It's the only stage that understands C as a *language*: it parses the code, type-checks it, optimizes it, and emits assembly for your target CPU.

```asm
        .section        .rodata
.LC0:
        .string "%s %d\n"
.LC1:
        .string "Hello, world!"
        .text
        .globl  main
main:
        pushq   %rbp
        movq    %rsp, %rbp
        movl    $16, %edx          # SQUARE(4) folded to 16 at compile time
        leaq    .LC1(%rip), %rsi
        leaq    .LC0(%rip), %rdi
        movl    $0, %eax
        call    printf@PLT
        movl    $0, %eax
        popq    %rbp
        ret
```

Output is still **human-readable text**. Notice `call printf@PLT` — the compiler emits a call to a name it has never seen the body of. Resolving that is somebody else's problem (stage 4).

**Why you'd run this manually:** checking whether the optimizer did what you hoped, or understanding a performance mystery.

## Stage 3 — Assemble to object file (`gcc -c` / `as`)

```bash
gcc -c hello.s -o hello.o      # via the driver
as hello.s -o hello.o          # calling the assembler directly
```

The assembler is a fairly dumb, near-mechanical translator: each assembly mnemonic becomes its binary machine-code encoding. The output is an **ELF relocatable object file** — binary, not text, and not runnable.

The key thing an object file carries beyond raw machine code is a **symbol table**:

```bash
$ nm hello.o
0000000000000000 T main      # T = defined here, in the text section
                 U printf    # U = undefined, someone else must supply it
```

Addresses inside are placeholders. The file also contains *relocation entries*: notes saying "once you know where `printf` really lives, patch the 4 bytes at offset 0x1f."

**Why you'd run this manually:** this is the normal build unit. Every `.c` in a project compiles to its own `.o`, independently and in parallel. Change one file, recompile one object — that's what make/ninja are built around.

## Stage 4 — Link (`gcc` / `ld`)

```bash
gcc hello.o -o hello       # correct way
ld hello.o -o hello        # will fail — see below
```

The linker takes one or more object files plus libraries, and stitches them into a single executable:

- Merges all `.text`, `.data`, `.rodata` sections together
- Resolves every `U` symbol against a definition somewhere (`printf` → libc)
- Applies relocations, patching in the now-known addresses
- Writes the program entry point and the loader metadata

Calling `ld` directly fails with something like `undefined reference to '_start'`, because your program doesn't actually start at `main`. It starts at `_start` in the C runtime startup files (`crt1.o`, `crti.o`, `crtn.o`), which sets up the stack, runs global constructors, calls `main`, then calls `exit()` with your return value. `gcc` knows to pass all of that to `ld`; bare `ld` doesn't.


## Summary table

| Stage | Command | Input | Output | Text or binary? |
|---|---|---|---|---|
| Preprocess | `gcc -E` | `.c` | `.i` | text (still C) |
| Compile | `gcc -S` | `.i` | `.s` | text (assembly) |
| Assemble | `gcc -c` | `.s` | `.o` | binary (relocatable) |
| Link | `gcc` | `.o` | executable | binary (runnable) |


## Which stage broke?

Error messages tell you where you are, which makes this genuinely useful knowledge:

| Error message | Stage | Usual fix |
|---|---|---|
| `fatal error: foo.h: No such file or directory` | Preprocessor | Fix your `-I` include paths |
| `error: expected ';' before ...`, `incompatible types` | Compiler | Fix the C |
| `Error: no such instruction` | Assembler | Bad inline asm, or wrong `-march` |
| `undefined reference to 'sqrt'` | Linker | Missing library (`-lm`) or object file |
| `multiple definition of 'x'` | Linker | Symbol defined in two translation units |


## Handy shortcuts

```bash
gcc -save-temps hello.c -o hello   # build normally, but keep .i, .s, .o
gcc -v hello.c -o hello            # show the actual cc1/as/collect2 invocations
```

**C++:** substitute `g++`; the preprocessed extension is `.ii` and the compiler proper is `cc1plus`. The rest of the pipeline is identical.

---

# Where Your Variables Actually Live: `.text`, `.rodata`, `.data`, `.bss`

When the compiler emits an object file, it doesn't produce one undifferentiated blob of bytes. It sorts everything you wrote into **sections** — named buckets, each with its own permissions and its own answer to the question *"do the initial bytes need to be stored in the file?"*

Two questions decide which bucket a thing lands in:

1. **Can it change at runtime?** → writable or read-only
2. **Are the initial bytes anything other than zero?** → stored in the file, or just a recorded size

That's the whole model:

| Section | Writable? | Executable? | Bytes stored in the file? |
|---|---|---|---|
| `.text` | no | **yes** | yes |
| `.rodata` | no | no | yes |
| `.data` | **yes** | no | yes |
| `.bss` | **yes** | no | **no** — size only |

Everything below is a consequence of that table.


## The running example

```c
#include <stdio.h>

int         counter   = 0;          // .bss    — initializer is zero
int         limit     = 100;        // .data   — nonzero initializer
const int   magic     = 0xCAFE;     // .rodata — const, known at compile time
char        buffer[4096];           // .bss    — uninitialized, 4 KB of nothing
char        greeting[] = "hello";   // .data   — mutable array, copy of the literal
const char *name      = "world";    // pointer in .data, the string in .rodata
static int  hits;                   // .bss    — static just changes linkage
static const double PI = 3.14159;   // .rodata

void bump(void) {                   // .text   — the machine code
    static int calls = 0;           // .bss    — static, not on the stack
    int scratch = 7;                // nowhere — lives on the stack at runtime
    calls++;
    counter++;
    (void)scratch;
}
```

Compile and inspect:

```bash
$ gcc -c demo.c -o demo.o
$ size demo.o
   text    data     bss     dec     hex filename
     78      22    4104    4204    106c demo.o
```

Note `bss` is 4104 bytes but the object file didn't grow by 4 KB. That's the point.

## `.text` — the code

**Read-only, executable, stored in the file.**

Everything that is machine instructions: function bodies, compiler-generated helpers, and on some targets small constant pools placed next to the code that uses them.

```bash
$ objdump -d demo.o --section=.text
0000000000000000 <bump>:
   0:   8b 05 00 00 00 00       mov    0x0(%rip),%eax
   6:   83 c0 01                add    $0x1,%eax
   ...
```

**Why read-only:** so a wild pointer or a buffer overflow can't rewrite your program while it's running. The kernel maps these pages `r-x`, and a write to them faults immediately.

**Why executable and nothing else is:** this is the **W^X** rule ("write xor execute"). No page should be both writable and executable, because that's exactly what an attacker needs to inject shellcode. `.text` gets execute, `.data` gets write, and never the twain.

**Why it's shareable:** because nothing ever modifies it, twenty copies of the same running binary map *the same physical pages* of `.text`. Twenty `bash` processes cost one copy of bash's code in RAM.

## `.rodata` — read-only data

**Read-only, non-executable, stored in the file.**

What ends up here:

- String literals: `"hello"`, `"%s %d\n"`
- `const`-qualified globals with compile-time-known values
- Jump tables generated from big `switch` statements
- Floating-point and vector constants the instruction set can't encode inline
- Anonymous constant aggregates the optimizer decided to materialize

```bash
$ objdump -s -j .rodata demo.o
Contents of section .rodata:
 0000 fe ca 00 00 00 00 00 00  6e 86 1b f0 f9 21 09 40   ........n....!.@
 0010 776f 726c 6400                                     world.
```

You can see `0xCAFE` little-endian, the double for `PI`, and the bytes of `"world"`.

**Why a separate section from `.text`:** both are read-only, but data is not code. Keeping constants out of the executable mapping means an attacker who tricks your program into jumping into your string table hits a non-executable page and dies instead of executing your data as instructions.

**Why a separate section from `.data`:** same sharing benefit as `.text`, plus real protection. This is the reason for the single most common beginner segfault:

```c
char *s = "hello";
s[0] = 'H';        // SIGSEGV — the literal lives in .rodata
```

versus:

```c
char s[] = "hello";
s[0] = 'H';        // fine — the array is a writable copy in .data
```

Both lines look almost identical. In the first, `s` is a pointer to read-only storage. In the second, the compiler stores the initializer bytes and *copies* them into an array you own. Write `const char *` for string literals and the compiler will catch the mistake for you.

### Where `const` does *not* mean `.rodata`

- `const int x = 5;` **inside a function** is an ordinary local. It lives on the stack (or in a register, or nowhere at all if the optimizer folds it away). `const` is a promise to the type checker, not a memory placement directive.
- `const char *p` means "pointer to const char" — the *pointee* is const, `p` itself is a mutable variable in `.data`/`.bss`. To put the pointer in read-only memory you need `const char *const p`.
- `const int t = time(NULL);` at file scope isn't a constant expression, so it can't be baked into a read-only section as literal bytes.

## `.data` — initialized, writable data

**Writable, non-executable, stored in the file.**

Globals and statics with a nonzero initializer. Every single byte of the initial value is physically present in the binary on disk, because the loader has to have something to copy into memory.

```bash
$ nm demo.o | grep -i ' d '
0000000000000000 D greeting
0000000000000008 D limit
0000000000000010 D name
```

**Why the bytes must be in the file:** there's no way to derive `100` or `"hello"` from thin air. Contrast with `.bss`, where "all zeros" is derivable.

**The cost:** `.data` is the section that makes binaries fat. A 1 MB lookup table with nonzero entries adds 1 MB to your executable. The same array left uninitialized adds ~0 bytes.

**Copy-on-write:** when the same binary runs twice, the loader initially shares the `.data` pages between processes. The moment either process writes to one, the kernel silently duplicates that single 4 KB page for the writer. You pay per modified page, not for the whole section up front.

### `.data.rel.ro` — the interesting middle case

```c
const char *const messages[] = { "a", "b", "c" };
```

This *should* be read-only — it's `const` all the way down. But the values are addresses, and in a position-independent executable (PIE, which is the default now) the compiler doesn't know those addresses until load time. So it can't be baked into `.rodata`.

The solution is a section called `.data.rel.ro`: writable during relocation, then the dynamic linker calls `mprotect()` to flip it to read-only before your `main` runs. This is **RELRO**, and it's why the GOT is harder to attack than it used to be.

```bash
$ readelf -lW ./a.out | grep GNU_RELRO
  GNU_RELRO      0x002d80 0x0000000000003d80 ... R
```

## `.bss` — uninitialized, writable data

**Writable, non-executable, *not* stored in the file — the file only records how big it is.**

The name is a fossil ("Block Started by Symbol", from a 1950s IBM assembler). Ignore the name; what matters is the behaviour.

Everything here is guaranteed to be zero when your program starts, and C's rules make this free: **static-storage-duration objects with no initializer, or an initializer of zero, are zero-initialized.** So the compiler doesn't need to store the bytes. It just says "reserve 4096 bytes, name it `buffer`" and moves on.

```bash
$ nm demo.o | grep -i ' b '
0000000000000000 b calls.0      # lowercase = static/local linkage
0000000000000000 B buffer
0000000000001004 B counter
0000000000001008 b hits
```

Proof that it's free:

```c
// big.c
char huge[100 * 1024 * 1024];   // 100 MB
int main(void) { return huge[0]; }
```

```bash
$ gcc big.c -o big && ls -l big
-rwxr-xr-x  16544 big          # 16 KB on disk, 100 MB of address space at runtime
```

Change it to `char huge[100*1024*1024] = {1};` and the binary becomes 100 MB. Same array, one nonzero byte, 6000× the disk usage — because now it's `.data`.

**Who zeroes it:**

- **On Linux/hosted systems:** nobody, really. The kernel maps those pages from `/dev/zero` (anonymous memory), which is zero-filled by definition — and lazily, so untouched pages cost no physical RAM at all.
- **On bare metal / embedded:** the startup code does it explicitly, with a loop that walks from `__bss_start` to `__bss_end` writing zeros before calling `main`. If you've ever written a linker script for a microcontroller, you've written this loop.

## Why this matters most on embedded targets

On a microcontroller the split stops being an abstraction and becomes a physical one: `.text` and `.rodata` stay in flash, `.data` and `.bss` must be in RAM.

```
FLASH (say 256 KB)        RAM (say 64 KB)
┌──────────────┐          ┌──────────────┐
│ .text        │          │ .data  ◄─────┼── copied from flash at boot
│ .rodata      │          │ .bss   ◄─────┼── zeroed by a loop at boot
│ .data (init  │──copy───►│ heap ↓       │
│  values)     │          │ stack ↑      │
└──────────────┘          └──────────────┘
```

Consequences that bite people:

- `.data` costs you **twice** — once in flash for the initial values, once in RAM for the live copy. A big initialized table burns both budgets. Marking it `const` moves it to `.rodata` and it stays in flash only.
- `.bss` costs RAM only, but it's charged at link time, not at runtime. If `.data + .bss + stack + heap` exceeds your RAM, the *linker* fails — you don't get to find out at runtime.
- The boot-time `.data` copy and `.bss` zeroing loop run before `main`. Anything you touch before that runs (an early ISR, for instance) sees garbage.

## Reading the tools

### `size` — the budget summary

```bash
$ size --format=SysV demo.o
section       size   addr
.text           78      0
.data           22      0
.bss          4104      0
.rodata         24      0
```

### `nm` — symbols, one letter per section

| Letter | Meaning |
|---|---|
| `T` / `t` | `.text` — code |
| `R` / `r` | `.rodata` — read-only data |
| `D` / `d` | `.data` — initialized writable data |
| `B` / `b` | `.bss` — zero-initialized data |
| `U` | undefined — needs the linker to resolve |
| `C` | common — legacy tentative definition |

**Uppercase = global** (visible to other translation units). **Lowercase = local** (`static`). That single case distinction tells you whether a symbol can collide at link time.

### `readelf` — full detail

```bash
readelf -SW demo.o        # section headers with flags
readelf -lW ./a.out       # program headers (segments) — what the loader sees
objdump -s -j .rodata x   # hex dump of one section
```

In the section flags, `A` = allocated into memory, `W` = writable, `X` = executable. `.text` is `AX`, `.rodata` is `A`, `.data` and `.bss` are `WA`. `.bss` has type `NOBITS` — the explicit ELF marker for "occupies no space in the file."

### Sections vs segments

Sections are for the **linker**. Segments (program headers) are for the **loader**. At link time, sections with matching permissions get merged into a handful of segments, because the MMU works in pages and you don't want a separate mapping per section:

```
.text + .rodata (+ .init, .plt)  →  one R-X / R-- LOAD segment
.data + .bss                     →  one RW- LOAD segment
```

This is why `.bss` is always placed immediately after `.data` — they share a segment, and the loader just maps extra zero pages past the end of the file-backed part.

## Not sections: the stack and the heap

Neither appears in the ELF file, because neither has a compile-time size.

- **Stack** — locals, parameters, return addresses. Created by the kernel at process start, grows downward automatically.
- **Heap** — `malloc`. Requested from the kernel at runtime via `brk`/`mmap`.

`int scratch = 7;` inside a function generates no section entry at all. It's a `mov` into a stack slot or a register, and it ceases to exist when the function returns.

---

## Quick placement lookup

| Declaration | Section | Why |
|---|---|---|
| `void f(void) {...}` | `.text` | code |
| `int g = 42;` | `.data` | nonzero initializer, writable |
| `int g = 0;` | `.bss` | zero is free |
| `int g;` | `.bss` | implicitly zero |
| `static int g = 5;` | `.data` | static changes linkage, not placement |
| `const int g = 5;` | `.rodata` | immutable, known at compile time |
| `char a[] = "hi";` | `.data` | writable array, initializer copied in |
| `char *p = "hi";` | `p` in `.data`, `"hi"` in `.rodata` | pointer is mutable, literal is not |
| `const char *const p = "hi";` | `.data.rel.ro` → read-only after startup | value is a relocatable address |
| `static const char *msgs[] = {...}` | `.data.rel.ro` | array of addresses |
| `int local = 3;` (in a function) | stack | automatic storage duration |
| `malloc(n)` | heap | runtime size |

## Practical takeaways

1. **Mark read-only tables `const`.** It moves them from `.data` to `.rodata`: smaller RAM footprint, shareable between processes, protected from stray writes, and on embedded it stays in flash entirely.
2. **Don't initialize globals to zero explicitly.** `int x = 0;` and `int x;` behave identically, but if the optimizer doesn't fold it, you've asked for file bytes you didn't need. (Modern GCC handles this; older and cross-compilers sometimes don't.)
3. **Unexpectedly huge binary?** Run `size`. A fat `.data` is almost always one big initialized array that should have been `const`.
4. **Unexpected segfault writing to a string?** You've got a `char *` pointing at a literal in `.rodata`. Compile with `-Wwrite-strings` to make the compiler warn.
5. **Out of RAM on a microcontroller?** `.data + .bss` is your static budget, and the linker will tell you the number before you ever flash the board.

<details>
<summary>macOS / Mach-O Addendum: Sections on Apple Silicon</summary>

## The name mapping

| Concept | ELF (Linux) | Mach-O (macOS) |
|---|---|---|
| Machine code | `.text` | `__TEXT,__text` |
| String literals | `.rodata` | `__TEXT,__cstring` |
| Other `const` data | `.rodata` | `__TEXT,__const` |
| Jump tables | `.rodata` | `__TEXT,__const` |
| Initialized writable | `.data` | `__DATA,__data` |
| Zero-initialized | `.bss` | `__DATA,__bss` |
| Tentative definitions | `.bss` / COMMON | `__DATA,__common` |
| Read-only after relocation | `.data.rel.ro` (RELRO) | `__DATA_CONST,__const` |
| GOT | `.got`, `.got.plt` | `__DATA_CONST,__got` |
| Thread-local | `.tdata` / `.tbss` | `__DATA,__thread_data` / `__thread_bss` |
| Unwind info | `.eh_frame` | `__TEXT,__unwind_info`, `__LD,__compact_unwind` (in `.o` only) |
| C++ static init | `.init_array` | `__DATA,__mod_init_func` |

## Handy commands

```bash
objdump -d toolchain.o                            # disassemble text — no -j needed
objdump -d --section=__TEXT,__text toolchain.o    # explicit
objdump -h toolchain.o                            # list all sections
objdump -s --section=__TEXT,__cstring toolchain.o # hex dump the strings
objdump --macho -l toolchain.o                    # Mach-O specific: load commands

-- otool — the native equivalent
otool -tV toolchain.o             # disassemble __text with symbolic operands
otool -l toolchain.o              # load commands: every segment and section, with sizes
otool -s __TEXT __cstring toolchain.o    # hex dump (note: space-separated, not comma)
otool -s __TEXT __const toolchain.o
otool -L ./prog                   # dynamic libraries this binary links against
otool -hv ./prog                  # Mach header and flags (PIE, TWOLEVEL, etc.)

nm -m toolchain.o                # prints the actual segment and section per symbol
size -m toolchain.o
```

## Quick command cheat sheet

| Task | Linux | macOS |
|---|---|---|
| Section sizes | `size --format=SysV x.o` | `size -m x.o` |
| List sections | `readelf -SW x.o` | `otool -l x.o` or `objdump -h x.o` |
| Disassemble | `objdump -d x.o` | `objdump -d x.o` or `otool -tV x.o` |
| Dump one section | `objdump -s -j .rodata x.o` | `otool -s __TEXT __const x.o` |
| Symbols with location | `nm x.o` | `nm -m x.o` |
| Dynamic libs | `ldd prog` | `otool -L prog` |
| Load-time segments | `readelf -lW prog` | `otool -l prog` |
| Trace linking | `LD_DEBUG=libs ./prog` | `DYLD_PRINT_LIBRARIES=1 ./prog` |
</details>
