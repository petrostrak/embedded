# Week 3

## Concepts

- [x] **`volatile` — what it stops the optimiser doing.** Every read in the source becomes a real load; every write becomes a real store. No caching in a register, no eliding, no reordering *of volatile accesses relative to each other*.
- [x] **`volatile` — what it does not give you.**
  - [x] Not atomicity. A `volatile uint32_t` read-modify-write is still three separate steps.
  - [x] Not ordering with respect to *non*-volatile accesses.
  - [x] Not a memory barrier — nothing is said to the CPU's store buffer or cache.
- [x] **`static` at file scope** — internal linkage. The closest thing C has to a lowercase Go identifier.
- [x] **`static` inside a function** — persistent storage, single instance, still scoped to the function. Note which section it lands in (Week 1 answer).
- [x] **`extern`** — declaration without definition; how a global is shared across translation units.
- [ ] **Header guards** (`#ifndef`/`#define`/`#endif`, or `#pragma once`) and *why* they're needed.
- [ ] **The one-definition rule.** Declarations in headers, definitions in exactly one `.c`.
- [ ] **`const` for flash placement.** Why a `const` array can live in `.rodata` and cost no RAM, and what breaks that (e.g. `const` pointer-to-non-const, or taking a mutable alias).
- [ ] **Undefined behaviour** — the largest single conceptual gap from Go. A Go program with a bug misbehaves; a C program with UB can have the bug *deleted by the optimiser*.
  - [ ] Strict aliasing — accessing an object through an incompatible pointer type.
  - [ ] Unaligned access.
  - [ ] Reading an uninitialised variable.
  - [ ] Out-of-bounds array access.
  - [ ] Shifting by ≥ the width of the type (and shifting a signed value into the sign bit).
  - [ ] Signed overflow (carried over from Week 1 — now see what the optimiser does with it).
- [ ] **Why no `malloc` on a 40 KB device.**
  - [ ] Fragmentation with no compaction and no MMU.
  - [ ] Non-deterministic allocation time — unacceptable in an ISR or a hard-real-time path.
  - [ ] No OOM story: what does your firmware *do* when `malloc` returns `NULL` at 3 a.m.?

## Project A — Make the optimiser lie to you

### Setup

- [ ] `flag.c` — defines a global flag and a function that sets it.
- [ ] `main.c` — polls the global in a loop, e.g. `while (!ready) { }`, then does something observable.
- [ ] Two translation units on purpose, so the compiler can't see the whole program.

### Observe the hoist

- [ ] Build at `-O0`:
  ```
  gcc -O0 -Wall -Wextra -Werror -std=c11 main.c flag.c -o poll_O0
  ```
- [ ] Build at `-O2`:
  ```
  gcc -O2 -Wall -Wextra -Werror -std=c11 main.c flag.c -o poll_O2
  ```
- [ ] `objdump -d poll_O0` and `objdump -d poll_O2` — save both listings.
- [ ] Find where `-O2` hoisted the load out of the loop. Record the loop body at each level.
  - [ ] Note whether `-O2` produced an infinite loop with *no memory access at all*.

### Add `volatile`

- [ ] Qualify the flag `volatile` in both the definition and the `extern` declaration.
- [ ] Rebuild at `-O2`, `objdump -d`, and diff against the previous listing.
- [ ] Write down the **exact instructions that changed** — not "it got slower", the actual load that reappeared inside the loop.
- [ ] Answer in a comment: what did `volatile` buy you here, and what bug would still exist if two writers incremented this flag?

## Project B — Three functions, three UBs

- [ ] Function 1: one specific UB (e.g. strict aliasing violation via a `float*`/`uint32_t*` pun).
- [ ] Function 2: one specific UB (e.g. shift by ≥ width, or signed overflow).
- [ ] Function 3: one specific UB (e.g. out-of-bounds read, or uninitialised read).
- [ ] Each function isolated, each returning something the caller prints so behaviour is observable.

### Two builds

- [ ] Sanitised:
  ```
  gcc -O1 -g -fsanitize=undefined,address -std=c11 ub.c -o ub_san
  ```
- [ ] Optimised, no sanitisers:
  ```
  gcc -O2 -std=c11 ub.c -o ub_O2
  ```
- [ ] Run both. Document, per function, where the behaviour diverges.

### The lesson case

- [ ] Include at least one case where **the optimiser removes a check you wrote** — e.g. a post-hoc overflow test, or a `NULL` check after a dereference.
- [ ] Confirm it in the disassembly: the branch is simply absent.
- [ ] Write the one-line takeaway: the compiler is allowed to assume UB never happens, so any code whose only purpose is to detect UB is dead code.
- [ ] Note why `-Wall -Wextra` did **not** save you here.

## Done when

- [ ] You can point at generated assembly and explain what `volatile` bought you, instruction by instruction.
- [ ] You can articulate why `volatile` is not `sync/atomic`, without hedging.
- [ ] You can name, from memory, the UB in each of your three functions and predict what `-O2` does to it.
- [ ] You can explain the no-`malloc` rule to someone who thinks it's just superstition.

---

<details>
<summary>volatile</summary> 

## what it stops the optimiser doing
The compiler normally treats your variables as pure bookkeeping. It is free to keep `x` in a register for a whole function, delete a store nobody reads back, or notice a loop condition never changes and hoist the load out. All of that is legal because the compiler assumes it is the only thing touching that memory.

`volatile` withdraws that assumption. It says: **this location can change or have effects outside the program's control flow, so do literally what I wrote.**

Guarantees:

- Every read in the source becomes a real load.
- Every write in the source becomes a real store.
- No caching in a register, no eliding, no reordering of volatile accesses **relative to each other**.

### Example 1 — the disappearing poll loop

```c
// Polling a hardware status register at a fixed address
uint32_t *status = (uint32_t *)0x4000A000;
while (*status == 0) { }     // -O2: load once, then `while (1);`
```

The compiler sees nothing in the loop body that could change `*status`, so it reads once and spins forever. Add the qualifier and every iteration emits a real load:

```c
volatile uint32_t *status = (volatile uint32_t *)0x4000A000;
while (*status == 0) { }     // reload each time
```

### Example 2 — the deleted store

Writing twice to a register is a common hardware idiom (pulse a bit, clear it). Without `volatile` the first store is dead code:

```c
*reg = 1;
*reg = 0;    // non-volatile: compiler deletes the first line entirely
```

### Example 3 — reads that *are* the side effect

Reading a UART data register pops a byte off the FIFO. Two reads must be two reads, not one read reused:

```c
volatile uint8_t *uart_rx = (volatile uint8_t *)UART_BASE;
uint8_t a = *uart_rx;   // byte 1
uint8_t b = *uart_rx;   // byte 2 — without volatile, b would just reuse a
```

### Syntax trap — the qualifier binds like `const`

```c
volatile uint32_t *p;           // pointer to volatile data  <-- what you want for MMIO
uint32_t * volatile p;          // volatile pointer, ordinary data
volatile uint32_t * volatile p; // both
```

## what it does not give you

This is where most of the real-world bugs live, because `volatile` *looks* like a concurrency tool and isn't one.

### Not atomicity

```c
volatile uint32_t counter;
counter++;    // load, add, store — three separate steps
```

An interrupt or another thread can land between any two of them and you lose an increment.

- On a 32-bit ARM the individual load is atomic in practice, but the **sequence** isn't.
- On an 8-bit AVR even the load isn't atomic — it's four byte-loads.

The qualifier says *do the access*; it says nothing about *indivisibly*.

### Not ordering with respect to non-volatile accesses

The standard only requires that volatile accesses happen in the written order **relative to each other**. An ordinary store can be moved across a volatile one:

```c
buffer[0] = 42;        // ordinary — may be sunk below the next line
flag = 1;              // volatile
```

So the classic "fill the buffer, then set the ready flag" pattern is **not** made safe by marking the flag volatile.

### Not a memory barrier

`volatile` emits no fence instruction.

- Nothing is said to the store buffer.
- Nothing invalidates a cache line.
- Nothing prevents the CPU from reordering the two accesses at runtime even if the compiler kept them in order.

On a multicore system another core can observe your stores out of order. For DMA on a non-coherent bus you additionally need explicit cache maintenance.

### Rule of thumb

> **`volatile` is for memory-mapped I/O. `_Atomic` is for threads.**

If two threads share data, use `_Atomic` / `atomic_load_explicit` / a mutex.

The one thread-adjacent exception: `volatile sig_atomic_t` for a flag set by a signal handler, and locals that must survive `longjmp`.

</details>

<details>
<summary>static</summary>

## at file scope — internal linkage

At file scope, `static` has **nothing to do with storage duration** (file-scope variables already live for the whole program). It controls **linkage** only: the symbol is not exported to the linker, so nothing outside this translation unit can name it.

```c
// counter.c
static int hits;                      // invisible to other .c files
static void bump(void) { hits++; }    // same for functions
```

Two different `.c` files can each define `static int hits;` with no collision. Drop the `static` and you get a duplicate-symbol link error.

### The Go analogy, adjusted

Go's privacy boundary is the **package**, which spans multiple files. C's is the **translation unit** — one `.c` file plus everything it `#include`s. So `static` is stricter: it's file-private, not package-private.

C has no direct equivalent of "visible to my sibling files but not to importers". The convention is a private header (`internal.h`) that only your own `.c` files include.

### Optimisation payoff

Because the compiler can see *every* use of a `static` function, it can:

- inline it aggressively, or
- delete it entirely if unused (`-Wunused-function` fires on exactly this).

A non-static function must be emitted in full, in case someone links against it.

## inside a function — persistent storage

Inside a function, `static` flips the **storage duration** instead: the variable lives for the entire program, but its *name* is only visible inside that block.

```c
int next_id(void) {
    static int id = 0;   // initialised once, before main
    return ++id;
}
```

### Three consequences

1. **Initialised once**, before `main` runs — not on each call. In C the initialiser must be a constant expression (unlike C++, which permits runtime init with a thread-safe guard).
2. **One instance, shared by everything** — all callers, all threads. This makes the function non-reentrant and not thread-safe. It's exactly why `strtok` and the old `localtime` are landmines, and why the `_r` variants exist.
3. **It's a global with a private name.** Same memory behaviour as a file-scope variable; only the scope differs.

### Which section it lands in

| Declaration | Section | Why |
|---|---|---|
| `static int id = 0;` | `.bss` | Zero-init needs no bytes in the image, just a reserved size; startup code memsets it |
| `static int id;` | `.bss` | Uninitialised statics are guaranteed zero |
| `static int id = 7;` | `.data` | The value 7 must be stored in the binary and copied to RAM at startup |
| `static const char *msg = "hi";` | `.rodata` (pointer often `.data`) | On an MCU, `.rodata` can stay in flash and never cost RAM |

Never the stack — that's the whole point.

### Confirming it

```console
$ nm --defined-only prog.o | grep id
0000000000000004 b id.0        # lowercase = local symbol, 'b' = .bss
```

Note the mangled name (`id.0`, or `id.1234`) — the compiler renames it so two functions can each have their own `static int id` in the same object file.
</details>

<details>
<summary>extern</summary>

## declaration without definition

- A **definition** allocates storage.
- A **declaration** just promises the thing exists and gives its type so the compiler can generate code.

`extern` is how you write the second without accidentally doing the first.

### The standard pattern

Exactly one definition, and a declaration in the header:

```c
/* config.h */
extern int g_max_retries;        // declaration — no storage

/* config.c */
int g_max_retries = 3;           // definition — the actual bytes

/* main.c */
#include "config.h"
if (attempts > g_max_retries) { ... }   // linker resolves the reference
```

### The classic mistake

Putting `int g_max_retries = 3;` in the header. Every `.c` that includes it now defines the symbol:

```
multiple definition of 'g_max_retries'
```

Older GCC (pre-10) papered over the un-initialised variant of this with `-fcommon`, silently merging *tentative definitions* like `int g_max_retries;`. GCC 10+ defaults to `-fno-common` and errors, which is the better behaviour.

### Two footnotes

- **Functions are `extern` by default.** `extern void foo(void);` in a header is legal but redundant — everyone writes `void foo(void);`. The keyword only earns its keep on variables.
- `extern` is the exact opposite pole from `static`: `static` says "this symbol stops here," `extern` says "this symbol lives elsewhere." Both are about linkage; only `static`-in-a-block is about lifetime.
</details>

## Quick reference tables - volatile, static, extern

### What each keyword actually controls

| Keyword / position | Linkage | Storage duration | Scope |
|---|---|---|---|
| `static` at file scope | Internal | Static (unchanged) | File |
| `static` inside a function | None | **Static** (changed) | Block |
| `extern` on a variable | External | Static | Wherever declared |
| (nothing) at file scope | External | Static | File, but exported |
| (nothing) inside a function | None | Automatic (stack) | Block |
| `volatile` | *not a linkage/storage keyword* — it's a type qualifier | | |

### `volatile` guarantees at a glance

| Property | Provided? |
|---|---|
| Every source read becomes a real load | Yes |
| Every source write becomes a real store | Yes |
| No reordering **between** volatile accesses | Yes (compile time) |
| No reordering vs. **non-volatile** accesses | No |
| Atomicity of read-modify-write | No |
| CPU memory barrier / fence | No |
| Cache or store-buffer effects | No |

### Choosing the right tool

| Situation | Use |
|---|---|
| Memory-mapped hardware register | `volatile` |
| Flag set by a signal handler | `volatile sig_atomic_t` |
| Local that must survive `longjmp` | `volatile` |
| Shared between threads | `_Atomic`, or a mutex |
| Ordering across cores / DMA | Explicit barriers + cache maintenance |

<details>
<summary>Header Guards</summary>

### The problem

`#include` is not clever. It is a literal copy-paste performed by the
preprocessor. If a header gets pasted in twice, the compiler sees everything in
it twice.

This happens constantly without anyone intending it:

```c
/* types.h */
typedef struct { int x, y; } Point;

/* sensor.h */
#include "types.h"

/* motor.h */
#include "types.h"

/* main.c */
#include "sensor.h"
#include "motor.h"   /* types.h now pasted twice */
```

The compiler now sees two `typedef struct { int x, y; } Point;` lines and
reports a redefinition error. Same for `struct` definitions, `enum`s, and (in
C++) classes and inline functions.

Note that *some* things can legally repeat. Pure function declarations
(`int foo(int);`) may appear many times, as long as they agree. So a header
containing only prototypes might survive double inclusion by luck. Don't rely
on luck.

### The classic fix

```c
#ifndef TYPES_H
#define TYPES_H

typedef struct { int x, y; } Point;

#endif /* TYPES_H */
```

Read it as: "if the symbol `TYPES_H` is not yet defined, define it and process
everything down to `#endif`." The first inclusion defines the macro; every
later inclusion sees it already defined and skips the whole body. The macro
itself carries no data — it exists purely as a flag meaning *this file has
already been seen in this translation unit*.

The name must be unique across your whole project, including any library you
pull in. A collision means one header silently vanishes and you get baffling
"unknown type" errors. Conventions that help:

- `PROJECT_MODULE_H` — e.g. `ACME_SENSOR_H`
- Include the path: `DRIVERS_SPI_SPI_H`
- Avoid leading underscores followed by a capital (`_TYPES_H`) — names like
  that are reserved for the implementation.

### `#pragma once`

```c
#pragma once

typedef struct { int x, y; } Point;
```

Same effect, one line, no name to invent, no chance of collision. It is not in
the C standard, but GCC, Clang, MSVC, IAR, Keil, and essentially every compiler
you'd meet supports it.

The trade-off: it works by identifying *the file*, and file identity is fuzzier
than it sounds. Symlinks, hard links, network shares, or the same header
reachable via two different include paths can make one file look like two,
defeating the pragma. Compilers use inode/device numbers or content hashes to
mitigate this, and in practice it almost always works.

**Pragmatic advice:** `#pragma once` for application code, the `#ifndef` idiom
for headers you ship to unknown compilers or that must be strictly portable.
Some codebases use both — belt and suspenders.

### What guards do *not* do

A guard is per translation unit. Compiling `a.c` and `b.c` are two independent
runs of the compiler, and the header is fully processed in each. Guards prevent
double-*inclusion*; they do nothing about the linker seeing the same
*definition* from two objects. That is the next topic.
</details>

<details>
<summary>The One-Definition Rule</summary>

## Declaration vs definition

This distinction is the whole game.

A **declaration** announces that a name exists and states its type. It
allocates nothing and generates no code. It is a promise to the compiler:
*trust me, this exists somewhere; here's how to use it.*

A **definition** actually creates the thing — reserves the storage or emits the
machine code.

```c
extern int counter;        /* declaration: exists somewhere */
int counter;               /* definition: here it is, reserve 4 bytes */

int add(int, int);         /* declaration: prototype only */
int add(int a, int b) {    /* definition: real code */
    return a + b;
}
```

You may **declare** a thing as many times as you like.
You may **define** it exactly once in the entire program.

### Why the rule exists

The linker's job is to resolve every "somewhere" into a concrete address. If
two object files each define `counter`, the linker has two candidate addresses
and no basis for choosing — so it refuses:

```
multiple definition of `counter'
first defined here
```

If *zero* object files define it:

```
undefined reference to `counter'
```

### The layout that follows

**Header** — declarations only. Anything that can be repeated harmlessly in
every file that includes it:

```c
/* counter.h */
#pragma once

extern int counter;                /* declaration */
void counter_increment(void);      /* declaration */
uint32_t counter_get(void);        /* declaration */

typedef struct { int x, y; } Point;   /* type definition — allowed, see below */
#define MAX_COUNT 100                 /* macro — preprocessor, no storage */
```

**Exactly one `.c`** — the definitions:

```c
/* counter.c */
#include "counter.h"

int counter = 0;                   /* definition: the storage lives here */

void counter_increment(void) {     /* definition: the code lives here */
    counter++;
}

uint32_t counter_get(void) {
    return counter;
}
```

### The classic mistake

Dropping a definition into a header:

```c
/* counter.h — WRONG */
#pragma once
int counter = 0;     /* definition in a header! */
```

Include this from three `.c` files and you get three definitions and a link
error. Header guards will not save you — each file was included exactly once,
and each one correctly produced a definition. The guard did its job perfectly;
the problem is architectural.

> **Historical wrinkle:** some toolchains accept `int counter;` — no
> initializer — in multiple files by merging them into one "common" symbol.
> This is a legacy extension, not standard C, and GCC 10+ rejects it by default
> with `-fno-common`. Don't depend on it.

### What's legal in a header, and why

The one-definition rule really has two halves. Things that occupy storage or
emit code — variables and non-inline functions — must be defined once per
*program*. Things that only teach the compiler about shapes and names may be
defined once per *translation unit*, which is why they belong in headers.

| Goes in a header | Reason |
|---|---|
| `extern` variable declarations | Declaration, no storage |
| Function prototypes | Declaration, no code |
| `struct` / `union` / `enum` definitions | Type info only; compiler needs the layout to generate code |
| `typedef` | Alias only |
| `#define` | Handled by the preprocessor, never reaches the linker |
| `static inline` functions | Each file gets its own private copy — legal, and standard practice for small helpers |

| Goes in exactly one `.c` | Reason |
|---|---|
| Variable definitions | Occupies RAM at one address |
| Function bodies | Occupies flash at one address |
| Initialized arrays and tables | Occupies memory |

### `static` as an escape hatch

`static` at file scope gives a definition **internal linkage** — the symbol is
invisible outside its translation unit. Ten files can each have their own
`static int retry_count;` with no conflict, because ten separate objects exist
and the linker never compares them.

This is why `static inline` in a header works: every including file gets a
private definition, and small ones usually get inlined away entirely.

But beware `static` on *data* in a header:

```c
/* config.h */
static int mode = 0;    /* every including file gets its OWN copy */
```

This compiles and links cleanly — and is almost always a bug. `a.c` sets
`mode = 1`; `b.c` still reads `0`, because they are different variables.
Silent, and painful to debug.
</details>

<details>
<summary>const and Flash Placement</summary>

### The memory picture

On a microcontroller, RAM is precious and flash is comparatively plentiful. A
typical part might have 512 KB of flash and 64 KB of RAM. Understanding which
section your data lands in is the difference between fitting and not fitting.

| Section | Lives in | Notes |
|---|---|---|
| `.text` | Flash | Executable code |
| `.rodata` | Flash | Read-only data: constants, string literals, lookup tables |
| `.data` | RAM (+ flash) | Initialized writable data — **costs both**; see below |
| `.bss` | RAM | Zero-initialized writable data; costs no flash |

The `.data` double cost catches people out. A writable variable with a non-zero
initial value needs RAM to live in *and* a copy of its initial value stored in
flash, because RAM contents are undefined at power-on. The startup code copies
flash → RAM before `main()` runs. So `.data` charges you twice.

### What `const` buys you

```c
const uint16_t sine_table[256] = { 0, 402, 804, /* ... */ };
```

Because this object can never legally be written, the toolchain places it in
`.rodata` — flash. Cost: 512 bytes of flash, **zero bytes of RAM**, and no
startup copy. The CPU reads it directly from flash at runtime.

Drop the `const`:

```c
uint16_t sine_table[256] = { 0, 402, 804, /* ... */ };
```

Now it is `.data`: 512 bytes of RAM *plus* 512 bytes of flash for the
initializer, plus startup copy time. One keyword, 512 bytes of RAM.

Scale that up. A font bitmap, a CRC table, a set of calibration curves — a few
kilobytes of forgotten `const` is a routine cause of "why won't this link, the
RAM is full."

> **Two caveats.**
>
> `const` in C means "this code may not write it," not "this is immutable." The
> placement in read-only memory is a consequence of the compiler being
> *permitted* to assume no writes, not a guarantee written into the standard.
> In practice, every embedded toolchain does exactly this.
>
> On Harvard-architecture parts — classic AVR, PIC, 8051 — plain `const` does
> *not* get you flash placement, because code and data live in separate address
> spaces the C model doesn't natively express. Those toolchains need `PROGMEM`
> (AVR), `__flash`, or similar, plus special accessors to read. Everything below
> assumes a flat address space (ARM Cortex-M, RISC-V), which is the common case
> today.

### What breaks it

#### `const` in the wrong place on a pointer

Pointer declarations read right-to-left, and the position of `const` changes
everything:

```c
const char *p;          /* pointer to const char — the POINTER is writable */
char *const p;          /* const pointer to char — the TARGET is writable */
const char *const p;    /* both read-only */
```

Applied to a table of strings:

```c
/* Only the strings are const; the array of pointers is writable */
const char *messages[] = { "OK", "FAIL", "BUSY" };
```

The string literals go to `.rodata`. But `messages` itself — an array of three
pointers — is a writable array with non-zero initial values, so it lands in
`.data`. On a 32-bit target that is 12 bytes of RAM plus 12 bytes of flash,
silently.

The fix is a second `const`:

```c
const char *const messages[] = { "OK", "FAIL", "BUSY" };
```

Now the array is itself read-only and joins the strings in flash. Zero RAM.

The same trap applies to tables of function pointers, structs containing
pointers, and any nested pointer type. **Every level needs its own `const`.** A
struct is only fully read-only if the struct object is `const` *and* its pointer
members are pointers-to-const.

```c
typedef struct {
    const char *name;        /* pointer-to-const, and... */
    void (*handler)(void);
    uint8_t id;
} command_t;

/* ...the array itself is const, so the whole thing is in flash */
static const command_t commands[] = {
    { "reset",  cmd_reset,  1 },
    { "status", cmd_status, 2 },
};
```

#### Taking a mutable alias

If you create a non-const pointer to the object, or pass its address to
something that might write, the object may need to be writable — and the
compiler may relocate it out of `.rodata`:

```c
const uint8_t config[16] = { /* ... */ };

uint8_t *p = (uint8_t *)config;   /* casting away const */
p[0] = 0xFF;                      /* UNDEFINED BEHAVIOUR */
```

Two things go wrong here. First, writing through a cast-away-`const` pointer to
an object that was *defined* `const` is undefined behaviour, full stop. Second,
on real hardware, what actually happens is usually worse than a compile error:
the object is in flash, the store instruction fails silently or triggers a bus
fault, and you spend an afternoon debugging. Some parts require an
unlock-erase-program sequence and simply ignore stray writes.

Related traps:

- **Non-const parameters.** Passing `const` data to
  `void process(uint8_t *buf)` requires a cast to compile, and that cast is a
  lie. Declare read-only parameters `const uint8_t *buf` — it documents intent
  and lets the compiler catch the mistake.
- **Address taken and escaping.** If the address of a `const` object is passed
  somewhere the compiler cannot analyse, it must keep the object as a real
  addressable thing, which limits some optimizations — though it can still stay
  in `.rodata`.

#### `const` on a local variable

```c
void f(void) {
    const int scale = 100;                    /* likely folded into an immediate */
    const uint8_t table[64] = { /* ... */ };  /* may be built on the STACK each call */
}
```

A `const` local is still an automatic object with automatic storage duration.
For a scalar, the optimizer will almost certainly fold the value into the
instruction stream and it costs nothing. For an array, the compiler often
copies the initializer from flash onto the stack every time the function is
entered — cost paid in stack space *and* cycles, per call.

Add `static` to hoist it out of the stack and into `.rodata`:

```c
static const uint8_t table[64] = { /* ... */ };
```

`static const` at file or block scope is the reliable idiom for read-only
tables. The `static` also gives internal linkage, so the symbol stays private
and the linker can discard it if unused (especially with
`-ffunction-sections -fdata-sections -Wl,--gc-sections`).

#### `const volatile` — a different animal

```c
const volatile uint32_t *const STATUS_REG = (uint32_t *)0x40000000;
```

This is not about flash. `const` means *your* code won't write it; `volatile`
means the value can change outside the program's control (hardware sets it) so
the compiler must not cache or elide reads. Correct for a read-only hardware
status register. It will not be placed in `.rodata` — the address is fixed by
the peripheral map.

### Verifying it, rather than hoping

Never assume. Ask the toolchain.

```bash
# Section sizes: text = flash code+rodata, data = RAM w/ initializer, bss = RAM zeroed
arm-none-eabi-size -A firmware.elf

# Which section did a specific symbol land in?
arm-none-eabi-nm --print-size firmware.elf | grep sine_table
```

In the `nm` output, the letter code tells you immediately:

| Code | Meaning |
|---|---|
| `R` / `r` | `.rodata` — flash, no RAM. **What you want.** |
| `D` / `d` | `.data` — RAM plus flash initializer. |
| `B` / `b` | `.bss` — RAM, zeroed. |
| `T` / `t` | `.text` — code in flash. |

Uppercase means global (external linkage), lowercase means `static`. Seeing `D`
where you expected `R` is the signal that a `const` is missing or misplaced.

The linker map file (`-Wl,-Map=output.map`) gives the full picture: every
symbol, its address, its size, and its section. When RAM is unexpectedly full,
sorting the map by size in `.data` and `.bss` finds the culprit in about a
minute.

</details>

## Quick Reference

**Header guards** stop the preprocessor from pasting the same text twice into
one translation unit. Use `#pragma once`, or `#ifndef PROJECT_MODULE_H` when
strict portability matters.

**One-definition rule:** declarations in headers, definitions in exactly one
`.c`. Guards operate per translation unit and cannot prevent multiple
definitions across files — that is a design decision, not a preprocessor one.
Types, macros, and `static inline` functions are the legitimate exceptions that
belong in headers.

**`const` for flash:** `static const` on read-only tables moves them from
`.data` (RAM + flash + startup copy) to `.rodata` (flash only). Every level of a
pointer or nested type needs its own `const`, or the outer object stays writable
and stays in RAM. Never cast away `const` on a genuinely `const` object. Confirm
with `size` and `nm` rather than trusting that it worked.

### Cheat sheet

```c
/* --- header: declarations only --- */
#pragma once
extern int counter;                        /* declaration */
void do_thing(void);                       /* declaration */
typedef struct { int x, y; } Point;        /* type — fine in a header */
#define MAX 100                            /* macro — fine in a header */
static inline int sq(int v){ return v*v; } /* fine in a header */

/* --- exactly one .c: definitions --- */
int counter = 0;                           /* .data  — RAM + flash */
void do_thing(void) { /* ... */ }          /* .text  — flash */

/* --- read-only data: flash, zero RAM --- */
static const uint16_t table[256]      = { /* ... */ };  /* .rodata */
static const char *const msgs[]       = { "OK", "ERR" }; /* .rodata — note BOTH const */
static const char *msgs_bad[]         = { "OK", "ERR" }; /* .data — RAM wasted! */
```

| Symptom | Likely cause |
|---|---|
| `redefinition of 'X'` | Missing header guard |
| `multiple definition of 'x'` | Definition placed in a header |
| `undefined reference to 'x'` | Declared but never defined, or `.c` not linked |
| Value set in one file, unseen in another | `static` data in a shared header |
| RAM full, flash mostly empty | Missing `const` on lookup tables |
| Symbol shows `D` in `nm`, expected `R` | `const` missing at an outer pointer level |
| Write to a "constant" silently does nothing | Cast away `const`; object is in flash |
