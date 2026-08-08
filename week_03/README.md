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
- [x] **Header guards** (`#ifndef`/`#define`/`#endif`, or `#pragma once`) and *why* they're needed.
- [x] **The one-definition rule.** Declarations in headers, definitions in exactly one `.c`.
- [x] **`const` for flash placement.** Why a `const` array can live in `.rodata` and cost no RAM, and what breaks that (e.g. `const` pointer-to-non-const, or taking a mutable alias).
- [ ] **Undefined behaviour** a C program with UB can have the bug *deleted by the optimiser*.
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

<details>
<summary>Undefined Behaviour</summary>

## The core idea

The C standard divides program behaviour into a few buckets. The important one here is **undefined behaviour (UB)**: constructs for which the standard imposes *no requirements at all*. Not "returns a garbage value", not "crashes" — literally no requirements.

The consequence that catches people out is not that UB *might* do something bad. It is that the optimiser is **allowed to assume UB never happens**, and it uses that assumption to rewrite your code.

So the reasoning goes:

1. You write a check, or a loop bound, or a null test.
2. The compiler notices that the only way to reach that code is via a path that would have been UB.
3. Since UB "cannot happen", that path cannot happen.
4. Therefore the check is dead code.
5. The check is deleted.

**Your bug report gets optimised away, and the bug stays.**

This is why UB behaves so maliciously in practice: it usually works at `-O0`, works in the debug build, works in your unit test, and then misbehaves in the release build on a different compiler version. The code did not change. The *assumptions the optimiser was licensed to make* changed.

A useful mental model: **UB is a promise you make to the compiler.** `a + b` on signed ints is you promising "this will not overflow". If you break the promise, the compiler is not lying to you — you lied to it first.

## Strict aliasing

### The rule in plain English

An object in memory has an *effective type*. You are only allowed to read or write it through a pointer whose type is compatible with that effective type (roughly: the same type, a signed/unsigned variant of it, or `char`/`unsigned char`).

The compiler exploits this to conclude **"an `int*` and a `float*` cannot point at the same bytes"**, and therefore that a write through one cannot affect a read through the other. That lets it reorder loads and stores, cache values in registers, and skip reloads.

### Broken example — the classic "fast inverse square root" style type pun

```c
float bits_to_float(int i) {
    return *(float *)&i;       /* UB: reading an int object as a float */
}

int float_to_bits(float f) {
    return *(int *)&f;         /* UB: reading a float object as an int */
}
```

### Broken example where the optimiser visibly eats your work

```c
int f(int *pi, float *pf) {
    *pi = 1;
    *pf = 2.0f;    /* if these actually alias, this overwrites *pi */
    return *pi;    /* compiler assumes still 1, does NOT reload from memory */
}
```

Called as `f(&x, (float*)&x)`, the "obvious" answer is whatever bit pattern `2.0f` is. The compiler is entitled to emit `return 1;` and never touch memory again. The reload — the thing that would have shown you the aliasing — is deleted.

### Broken example — the "header punning" pattern

```c
struct header { uint32_t len; uint32_t type; };

size_t get_len(char *buf) {
    struct header *h = (struct header *)buf;   /* effective type of buf is char, not header */
    return h->len;
}
```

This one is *extremely* common in networking and parser code, and it is UB twice over — strict aliasing **and** alignment (see §2).

### The fix: `memcpy`

```c
float bits_to_float(uint32_t i) {
    float f;
    memcpy(&f, &i, sizeof f);
    return f;
}
```

`memcpy` is the blessed escape hatch. It is not slow: every mainstream compiler recognises a fixed-size `memcpy` and lowers it to a single register move or load. You get the reinterpretation you wanted with none of the UB.

Other legitimate routes:

- **Unions** — `union { uint32_t u; float f; } u; u.u = bits; return u.f;` is well-defined in C (this is *not* true in C++).
- **`unsigned char*` / `char*`** — always allowed to alias anything. This is why byte-wise inspection of an object is legal.
- **`-fno-strict-aliasing`** — turns the optimisation off (Linux kernel does this). A build-flag band-aid, not a portability fix.

## Unaligned access

### The rule in plain English

Every type has an alignment requirement — a `uint32_t` typically must live at an address divisible by 4. Creating a pointer to a type at an address that does not satisfy its alignment is UB **at the moment you form the pointer**, before you even dereference it.

### Broken example

```c
uint32_t read_u32(unsigned char *p) {
    return *(uint32_t *)(p + 1);   /* p+1 is almost certainly not 4-aligned */
}
```

### Why "but it works on x86" is a trap

x86-64 tolerates unaligned scalar loads in hardware, so this appears to work. Three things can still break it:

- **ARM, RISC-V, SPARC, MIPS** — may fault outright, or silently load from the *rounded-down* address, giving you wrong data with no crash.
- **Vectorisation.** The optimiser sees an aligned `uint32_t*`, decides a loop over it can use SIMD, and emits an instruction like `movaps` that *does* require alignment. Your scalar loop worked; the auto-vectorised version segfaults. Nothing about your source changed.
- **Alignment-based reasoning.** The compiler knows the low bits of an aligned pointer are zero and may fold that into address arithmetic or pointer-tagging assumptions.

### The fix: `memcpy` again

```c
uint32_t read_u32(const unsigned char *p) {
    uint32_t v;
    memcpy(&v, p, sizeof v);
    return v;
}
```

Or build the value explicitly, which also fixes endianness portability:

```c
uint32_t read_u32_be(const unsigned char *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}
```

Note how the `memcpy` fix solves strict aliasing and alignment simultaneously. That is not a coincidence — both rules exist to let the compiler reason about typed memory, and `memcpy` is the operation that means "just move bytes, make no claims".

## Reading an uninitialised variable

### The rule in plain English

An uninitialised automatic variable has an **indeterminate value**. That is not "some specific unknown number". It is a value the compiler may treat as *whatever is most convenient at each point of use* — it need not be consistent between two reads of the same variable, because the compiler never has to commit it to memory at all.

### Broken example — a variable that is two values at once

```c
#include <stdio.h>

void f(void) {
    int x;                 /* never initialised */
    if (x > 10) puts("big");
    if (x <= 10) puts("small");
}
```

You expect exactly one line. You can legitimately get **both**, or **neither**. Each comparison can be independently constant-folded, because there is no requirement that they agree.

### Broken example — the "leaks a secret" case

```c
struct packet p;         /* not zeroed */
p.type = 1;
p.len  = 4;
send(fd, &p, sizeof p, 0);   /* padding + unset fields = old stack contents on the wire */
```

This is a real and repeated class of security bug (the Heartbleed family of "uninitialised memory disclosure"). Note that **padding bytes are indeterminate even if you initialise every named member**, so `memset` or `= {0}` is the only reliable answer here.

### Broken example — the poisoned bool

```c
_Bool b;                 /* uninitialised */
if (b) { /* ... */ } else { /* ... */ }
```

A `_Bool` is required to hold only 0 or 1. If the garbage byte is `0x2A`, you have an object holding a value its type cannot represent — a *trap representation*. Downstream code that assumes `b` is 0-or-1 (e.g. using it as an array index into a 2-element table) is now doing something arbitrary.

### The fix

Initialise at the point of declaration.

```c
int x = 0;
struct packet p = {0};       /* zeroes padding too */
```

Compilers help here if you ask: `-Wuninitialized -Wmaybe-uninitialized -Werror`. Note that `-Wuninitialized` needs optimisations on in GCC to be effective, since the analysis rides on the optimiser's dataflow. There is also `-ftrivial-auto-var-init=zero` (Clang and modern GCC), which does not make the code correct but makes the failure deterministic.

## Out-of-bounds array access

### The rule in plain English

You may index an array from `0` to `n-1`. You may also form (but not dereference) the one-past-the-end pointer, so that `for (p = a; p != a + n; ++p)` is legal. Anything beyond that — indexing, or even *computing* the address — is UB.

Crucially, the compiler assumes in-bounds access and propagates that backwards.

### Broken example — the off-by-one

```c
int a[10];
for (int i = 0; i <= 10; i++)   /* writes a[10] */
    a[i] = i;
```

The nasty version of this: `a[10]` may land on the loop counter `i` itself, resetting it and giving you an infinite loop. Or the compiler, knowing `a[i]` is in bounds, concludes `i < 10` always holds, and therefore `i <= 10` is always true — and removes the exit test entirely.

### Broken example — the check the compiler deletes

```c
int table[4];

int get(int i) {
    int v = table[i];        /* compiler: therefore 0 <= i < 4 */
    if (i < 0 || i >= 4)     /* ...therefore this is dead code */
        return -1;
    return v;
}
```

The bounds check is removed. This is the canonical shape of the problem: **you wrote the safety check after the access, so the access licensed the compiler to delete the check.** The same pattern with a null test — dereference first, check for null second — is exactly the Linux kernel `tun` driver CVE-2009-1897.

### Broken example — pointer arithmetic alone

```c
int a[10];
int *p = a + 20;        /* UB already, no dereference needed */
```

### The fix

Check before you access, and make the bound the single source of truth.

```c
int get(int i) {
    if (i < 0 || (size_t)i >= sizeof table / sizeof table[0])
        return -1;
    return table[i];
}
```

For loops, prefer `size_t` counters and `< n`, or iterate over pointers. Where the language offers a checked container idiom, use it.

## Shifting by ≥ the width of the type

### The rule in plain English

For `x << n` and `x >> n`, it is UB if `n` is negative, or if `n` is greater than or equal to the width in bits of the *promoted* type of `x`.

The "promoted" part is the subtle bit: operands narrower than `int` are promoted to `int` first, so shift counts are checked against `int`'s width, not the original type's.

### Broken example — the mask that isn't

```c
uint64_t mask(int n) {
    return (1 << n) - 1;    /* 1 is int: UB for n >= 32, even though result is uint64_t */
}
```

This produces a correct-looking mask up to 31 and then falls off a cliff. The fix is to make the *shifted operand* wide enough:

```c
uint64_t mask(int n) {
    return ((uint64_t)1 << n) - 1;   /* well-defined for n < 64 */
}
```

### Broken example — the rotate

```c
uint32_t rotl(uint32_t x, unsigned n) {
    return (x << n) | (x >> (32 - n));   /* n == 0 gives x >> 32: UB */
}
```

The fix, which compilers recognise and turn into a single `rol` instruction:

```c
uint32_t rotl(uint32_t x, unsigned n) {
    return (x << (n & 31)) | (x >> (-n & 31));
}
```

### What the optimiser does

Shift-by-too-much is UB partly because hardware disagrees: x86 masks the count to the low 5 or 6 bits (`1u << 32` gives `1`), while ARM saturates to zero (`1u << 32` gives `0`). Since the standard refuses to pick a winner, the compiler is free to constant-fold a compile-time-visible over-shift to whatever it likes — and, more importantly, to assume `n < 32` everywhere downstream, deleting your `if (n >= 32)` guard.

### The signed sign-bit case

Historically, `1 << 31` on a 32-bit `int` was also UB: the result is not representable in `int`, and left-shift of a signed value was specified in terms of the arithmetic value `x * 2^n`. So:

```c
int x = 1 << 31;    /* UB in C99/C11 — use 1u << 31 */
```

Left-shifting a *negative* value was likewise UB. C23 has since defined signed left shift as a plain bit shift on the two's-complement representation, and C23 mandates two's complement. But if your code must build under C11 or older, or under `-std=c99`, treat this as UB and reach for unsigned:

```c
uint32_t x = 1u << 31;    /* always fine */
```

Right-shifting a negative signed value is *implementation-defined*, not undefined — in practice always arithmetic shift, but still worth avoiding in portable code.

**Rule of thumb: do bit manipulation on unsigned types.** Unsigned arithmetic is defined to wrap, unsigned has no sign bit to overflow into, and unsigned right shift is unambiguous.

## Signed integer overflow — and what the optimiser does with it

### The rule in plain English

If a signed arithmetic operation produces a result outside the range of its type, the behaviour is undefined. Unsigned overflow is *defined* (it wraps modulo 2^N); signed overflow is not.

The hardware almost certainly wraps. That is irrelevant. The optimiser does not model the hardware here — it models the standard, and the standard says overflow cannot happen. So the compiler gets to reason as if signed integers were **mathematical integers with no upper bound**.

### The deleted check, exhibit A

```c
int will_overflow(int a) {
    return a + 100 < a;    /* "did adding 100 wrap?" */
}
```

Mathematically, `a + 100 < a` is false for all `a`. The compiler applies exactly that reasoning and emits `return 0;`. Your overflow check is now a function that never detects overflow. This is not a hypothetical — it is what GCC and Clang do at `-O2`.

### The deleted check, exhibit B

```c
int is_negative_abs(int x) {
    return abs(x) < 0;     /* true for INT_MIN, since -INT_MIN overflows */
}
```

`abs` is documented to return a non-negative value, so the compiler folds this to `0` — and the one input where it genuinely misbehaves, `INT_MIN`, is precisely the UB case.

### The infinite loop

```c
for (int i = 1; i > 0; i *= 2)
    do_something();
```

You wrote this expecting it to terminate when `i` overflows to a negative value. The compiler reasons: `i` starts positive, doubling a positive number keeps it positive, therefore `i > 0` is invariant, therefore the exit test is dead. It emits an unconditional infinite loop.

### The promoted loop bound

```c
void f(int n) {
    for (int i = 0; i <= n; i++)   /* n == INT_MAX means i++ overflows */
        g(i);
}
```

Because `i` cannot overflow, the compiler may promote `i` to a 64-bit register and skip the wraparound handling — so with `n == INT_MAX` the loop runs forever rather than wrapping.

### Why the compiler is allowed to be this aggressive

Assuming no signed overflow is what enables genuinely valuable optimisations: strength reduction, promoting 32-bit induction variables to native 64-bit registers, proving loops terminate, simplifying array index arithmetic. Compiler authors are unwilling to give that up, so the assumption stays.

### The fix

Check for overflow **without** overflowing:

```c
#include <limits.h>

bool add_overflows(int a, int b) {
    if (b > 0 && a > INT_MAX - b) return true;
    if (b < 0 && a < INT_MIN - b) return true;
    return false;
}
```

Or use the builtins, which compile to a single add plus a flag test:

```c
int r;
if (__builtin_add_overflow(a, b, &r)) { /* handle */ }
```

C23 adds `<stdckdint.h>` with `ckd_add`, `ckd_sub`, `ckd_mul` for the same purpose. Alternatively, cast to unsigned to get defined wrapping, or build with `-fwrapv` to make signed overflow wrap by definition — with the caveat that you have then written code that only works on your compiler's dialect of C.

## Detecting all of this

Static analysis is not enough — most of these need runtime instrumentation.

**UndefinedBehaviorSanitizer** catches signed overflow, bad shifts, unaligned access, and more:

```sh
cc -fsanitize=undefined -fno-sanitize-recover=all -g -O1 prog.c
```

Useful individual checks: `-fsanitize=signed-integer-overflow,shift,alignment,bounds,object-size,null,integer-divide-by-zero`.

**AddressSanitizer** catches out-of-bounds on heap, stack, and globals:

```sh
cc -fsanitize=address -g -O1 prog.c
```

**MemorySanitizer** (Clang only) catches uninitialised reads — the one UBSan will not find:

```sh
clang -fsanitize=memory -fsanitize-memory-track-origins -g -O1 prog.c
```

**Warnings worth having on by default:**

```sh
-Wall -Wextra -Wstrict-aliasing=2 -Wcast-align -Wuninitialized -Wshift-overflow=2 -Wshift-count-overflow -Warray-bounds=2
```

**Practical habits, in rough order of value:**

1. Run your test suite under UBSan and ASan in CI, not just locally. Sanitizers only report what you actually execute, so coverage matters.
2. Test at `-O0` **and** `-O2`. A behaviour difference between optimisation levels is a very strong UB smell.
3. Test with both GCC and Clang. They exploit different UB in different places.
4. Reach for `memcpy` whenever you want to reinterpret bytes.
5. Do bit manipulation on unsigned types.
6. Check bounds and nullness *before* the access, never after.
7. Initialise on declaration.

## The one-line summary

UB is not "the program does something weird". UB is **"the compiler is allowed to assume this line is unreachable"** — and it will use that assumption to delete the very code you wrote to catch the problem. Treat every UB construct as a promise to the optimiser, and only make promises you can keep.

</details>

<details>
<summary>Why No malloc on a 40 KB Device</summary>

## The core idea

On a desktop, `malloc` is nearly free of consequences. You have gigabytes, an MMU that gives every process a private virtual address space, a kernel that can page things out, and an OOM killer that takes the blame when it all goes wrong. If allocation is occasionally slow, nobody notices.

On a 40 KB microcontroller you have **none of that**. No MMU, no virtual memory, no swap, no supervisor to fall back on, and a total budget smaller than the icon on a desktop shortcut. The three properties that make `malloc` acceptable on a big machine are exactly the three you have lost.

The result is that `malloc` stops being a convenience and becomes a source of failures that are **rare, timing-dependent, and impossible to reproduce** — the worst possible combination. The device runs fine on your desk for a week and then bricks itself in the field after eleven days of uptime.

A useful framing: **on a small device, memory is a static resource to be budgeted at build time, not a dynamic resource to be negotiated at run time.** Once you accept that, most of the reasons below become obvious consequences.

## Fragmentation with no compaction and no MMU

### The problem in plain English

Fragmentation is when you have enough total free memory but not enough *contiguous* free memory. You ask for 1 KB, there is 6 KB free, and the request still fails because the free space is scattered in 200-byte pieces between live allocations.

On a desktop this is survivable for two reasons: there is so much headroom that fragmentation rarely bites, and the MMU means the OS can hand a process pages from anywhere in physical RAM and make them look contiguous in the process's virtual address space. **A microcontroller has neither.** A physical address is the only address. If the free bytes are not physically adjacent, they cannot serve your request, full stop.

And there is no compaction. A garbage-collected runtime can move live objects to squeeze the holes out. C cannot: you handed out raw pointers, those pointers are held in structs and locals and possibly in an ISR's working state, and nothing may relocate them. **The holes are permanent.**

### Worked example

Say you have 8 KB of heap and a simple sequence of message handling:

```c
uint8_t *a = malloc(1024);   /* [a:1024                    ] free: 7168 */
uint8_t *b = malloc(64);     /* [a:1024][b:64              ] free: 7104 */
uint8_t *c = malloc(1024);   /* [a:1024][b:64][c:1024      ] free: 6080 */
free(a);                     /* [ 1024 ][b:64][c:1024      ] free: 7104 */
```

You have 7104 bytes free. You now ask for a 2 KB buffer. The 1024-byte hole where `a` was is too small. The tail is large enough, so this succeeds — this time.

Now run that pattern a few hundred thousand times with varying sizes, which is what a device does over days of uptime. The `b`-sized long-lived allocations act as **pins**, and the heap ends up as a comb: live 64-byte object, hole, live 64-byte object, hole. Total free memory looks healthy at 5 KB. The largest single block is 300 bytes. Your next 1 KB request fails.

### Why this is so hard to test out

Fragmentation is a function of the **order and lifetime pattern** of allocations, not the amount. That means:

- It depends on the exact sequence of external events — packet arrival order, sensor timing, user button presses.
- It gets worse monotonically with uptime, so a 10-minute test tells you nothing about an 11-day deployment.
- It is not reproducible. The same firmware with the same inputs in a different order fragments differently.
- Adding a feature elsewhere can change the pattern and shift the failure to a completely different allocation site.

This is the single strongest practical argument. **You cannot prove by testing that a fragmenting system will not fail**, and on a medical or safety-relevant device, "we tested it for a while and it seemed fine" is not an argument you can put in front of an auditor.

### The related hazard: heap and stack growing into each other

The classic small-MCU memory map has the heap growing up from the end of `.bss` and the stack growing down from the top of RAM, with a shared gap between them.

```
0x20000000  .data  .bss   heap -->            <-- stack   0x2000A000
```

Without an MMU there is **no guard page**. If the stack grows into the heap, it silently overwrites your allocated objects. If the heap grows into the stack, `malloc` may hand you memory the stack is about to use. Either way there is no fault, no trap, and no diagnostic — just corruption that surfaces later as a wild pointer or a nonsense sensor reading. Static allocation removes the ambiguity: the linker knows exactly how much RAM is committed, and the entire remainder is stack.

*Mitigations if you must have a gap:* an MPU region configured as a no-access guard band (most Cortex-M3 and above have one), or stack painting — fill the stack with a known pattern at boot and periodically check the high-water mark.

## Non-deterministic allocation time

### The problem in plain English

`malloc` does not take a fixed amount of time. A typical implementation walks a free list looking for a block that fits, and how long that takes depends on how many blocks are on the list and where the fit happens to be. It may also coalesce adjacent free blocks, split an oversized block, or extend the heap.

So the cost is somewhere between "a dozen instructions" and "hundreds of instructions, unbounded in principle". In real-time terms: `malloc` has **no useful worst-case execution time (WCET)**.

### Why that is fatal in an ISR or hard-real-time path

Hard real time means a deadline missed is a *failure*, not a slowdown. To make that guarantee you have to be able to add up the worst-case time of every step in the path and show the total fits inside the deadline. An operation with no WCET bound makes the sum unbounded, and the analysis collapses. It does not matter that the average case is fast — the average case is not what a guarantee is about.

Worse, `malloc` gets *slower as the heap gets more fragmented*, because the free list gets longer. So your timing degrades with uptime, in lockstep with the fragmentation problem. The system that met its deadlines in testing quietly stops meeting them on day nine.

### The reentrancy problem

There is a second, sharper issue in an ISR: `malloc` maintains global heap metadata, so it must protect it. Two things follow.

**It is generally not reentrant.** If your main loop is halfway through `malloc`, with the free list in an inconsistent intermediate state, and an interrupt fires and calls `malloc`, the second call sees corrupt metadata. The corruption may not manifest for hours.

```c
/* Don't do this. */
void ADC_IRQHandler(void) {
    sample_t *s = malloc(sizeof *s);   /* may reenter an in-progress malloc */
    if (!s) return;                    /* ...and now you're silently dropping samples */
    s->value = ADC->DR;
    queue_push(s);
}
```

**Making it reentrant makes it worse.** The standard fix is a lock, but in an ISR context a lock means either disabling interrupts (adding unbounded latency to *every other* interrupt in the system, including ones with tighter deadlines) or an RTOS mutex (which an ISR generally may not block on). Newlib's approach — `__malloc_lock`/`__malloc_unlock`, typically implemented as a global interrupt disable — means **any thread's `malloc` blocks every interrupt on the chip for its duration**. Your carefully tuned 10 µs interrupt latency is now hostage to heap fragmentation.

### The fix in a hot path: preallocate, or use a fixed-time allocator

Get the memory before you need it, outside the timing-critical path:

```c
/* Fixed pool, allocated at build time. Push/pop are O(1) and lock-free-able. */
static sample_t sample_storage[64];
static sample_t *free_list;

void pool_init(void) {
    for (size_t i = 0; i < 63; i++)
        sample_storage[i].next = &sample_storage[i + 1];
    sample_storage[63].next = NULL;
    free_list = &sample_storage[0];
}

sample_t *pool_alloc(void) {          /* O(1), bounded, ~5 instructions */
    sample_t *s = free_list;
    if (s) free_list = s->next;
    return s;
}

void pool_free(sample_t *s) {         /* O(1) */
    s->next = free_list;
    free_list = s;
}
```

This is a **fixed-block pool**, and it is the workhorse of embedded memory management. Every block is the same size, so:

- There is no fragmentation *by construction* — any free block satisfies any request.
- `alloc` and `free` are a handful of instructions with a hard WCET.
- The total footprint is visible in the linker map at build time.
- Failure is a full pool, which is a bounded, testable condition rather than an emergent one.

If you genuinely need variable sizes, a few pools of different sizes (say 32/128/512 bytes) covers most real workloads. If you need true general-purpose allocation with bounded time, **TLSF** (Two-Level Segregated Fit) is O(1) with a known constant and low fragmentation — but it is a considered engineering decision, not a default.

## No OOM story: what happens when `malloc` returns `NULL` at 3 a.m.?

### The problem in plain English

This is the question that usually ends the argument, because there is rarely a good answer.

On Linux, `malloc` failing is somebody else's problem — the OOM killer picks a victim, the process dies, systemd restarts it, and a log line appears. On a microcontroller, **you are the entire system.** There is no supervisor, no restart-and-carry-on, nobody watching. So: what does the code do?

Look at the shape of the code you have to write:

```c
result_t handle_message(const msg_t *m) {
    buffer_t *buf = malloc(m->len);
    if (!buf)
        return ERR_NOMEM;    /* ...and then what? */
    /* ... */
}
```

`return ERR_NOMEM` just moves the question up a level. Follow it all the way to the top and you end up at one of a small set of options, none of them good:

- **Ignore it.** Dereference `NULL`, which on most MCUs reads the vector table rather than faulting, so you get silent garbage instead of a clean crash. This is the worst outcome and, empirically, the most common.
- **Drop the work.** Lose the sample, discard the packet, skip the log entry. Sometimes acceptable — but you must be able to say *which* data you are willing to lose and prove the loss is safe.
- **Retry.** Retry what? Nothing is going to free memory on your behalf. If the cause was fragmentation, retrying will fail identically forever.
- **Reset the device.** Often the only honest answer. But a reset is a visible outage: a reboot mid-procedure, lost in-flight state, lost buffered results, an audit trail with a hole in it.
- **Degrade to a safe state.** The correct answer for a safety-relevant device, and also the most work. You need a defined reduced-function mode and you have to have designed it in advance.

### The three-in-the-morning part is the actual point

The failure will not happen during your test run. It will happen after days of uptime, on one unit out of five hundred, unattended, in a state you cannot reconstruct. You will get a report that says "the device stopped responding" and you will have no core dump, no `stderr`, possibly no persistent log, and no way to reproduce it.

Every allocation site is a potential failure point that has to be handled, tested, and reasoned about. On a device with a hundred `malloc` calls, that is a hundred error paths — and error paths are the least-tested code in any codebase. Most of that error-handling code has never executed even once.

### The fix: make the failure impossible instead of handled

If all memory is allocated statically at build time, **there is no `NULL` to check.** The linker either fits your program in RAM or it does not, and it tells you at build time, on your machine, with a clear error message. The 3 a.m. failure mode is converted into a compile error.

```c
/* Fails at build time, not at 3 a.m. */
static uint8_t rx_buffers[MAX_CONNECTIONS][MTU];
_Static_assert(sizeof rx_buffers < 16 * 1024, "rx buffers exceed RAM budget");
```

This is the deepest reason to avoid the heap on small devices. It is not that dynamic allocation is slow or wasteful. It is that **it moves a class of failure from build time to run time**, from your desk to the field, from deterministic to probabilistic.

## Secondary reasons worth knowing

**Code size.** A general-purpose `malloc`/`free` implementation is typically 1–3 KB of flash, plus per-allocation metadata overhead (commonly 4–8 bytes of header per block). On a 40 KB device, a fixed pool costs a few dozen bytes of code and zero per-block overhead.

**Every allocation is a leak waiting to happen.** A desktop process leaking 100 bytes per request is fine — it exits. Firmware never exits. Any leak, however small, is fatal given enough uptime; it is just a question of whether the device dies in a day or a year. Static allocation makes leaks structurally impossible.

**Certification and coding standards.** Dynamic allocation after initialisation is restricted or banned outright in most safety-relevant standards: MISRA C:2012 Directive 4.12 and Rule 21.3, DO-178C practice, IEC 61508, and the general expectation under IEC 62304 that resource usage be bounded and analysable. The reasoning in these standards is precisely the three points above — you cannot demonstrate a bound on memory or timing for a fragmenting, non-deterministic allocator.

**Debuggability.** A static memory map is a linker map file you can read. Every buffer has a name, a size, and an address that is the same on every boot and in every unit. Heap corruption gives you an address that means nothing and differs between runs.

## What to do instead

Roughly in order of preference:

1. **Static allocation.** `static` arrays sized by compile-time constants. The default. Add `_Static_assert` to enforce your budget.
2. **Stack allocation.** Fine for short-lived, bounded-size, non-escaping data. Avoid VLAs and `alloca` — they reintroduce unbounded, unchecked growth into a region with no guard page.
3. **Fixed-block pools.** When you need a variable *number* of same-shaped objects. O(1), no fragmentation, bounded footprint.
4. **Ring buffers.** The right structure for streaming data — sensor samples, UART bytes, log lines. Fixed footprint, O(1), and overflow is an explicit, testable condition rather than an allocation failure.
5. **Arena / region allocation.** Bump a pointer through a fixed static buffer, then reset the whole thing at once at a natural boundary (end of a request, end of a measurement cycle). Allocation is two instructions, there is no individual `free`, and fragmentation cannot accumulate because the arena is emptied wholesale.
6. **Allocate once at init, never free.** If you truly need `malloc`-shaped code, call it during startup only and never release. You get the convenience with none of the fragmentation, and failure happens at boot where it is visible and testable.
7. **A bounded-time allocator (TLSF or similar).** Last resort, deliberately chosen, with the WCET measured and the fragmentation bound documented.

Practical build-time hygiene: keep `-Wl,-Map=out.map` output under review so RAM growth is visible in code review, and consider linking without a heap at all (`-Wl,--wrap=malloc`, or simply providing a `malloc` that traps) so an accidental heap dependency pulled in by a library fails loudly rather than quietly working.

## The one-line summary

`malloc` on a big machine is safe because an MMU hides fragmentation, spare capacity hides timing variance, and the OS owns the failure. On a 40 KB device you have none of those, so the heap converts three build-time certainties — how much memory you use, how long an operation takes, and whether it can fail — into three run-time gambles that only lose after you have shipped.

</details>
