# Week 3

## Concepts

- [ ] **`volatile` — what it stops the optimiser doing.** Every read in the source becomes a real load; every write becomes a real store. No caching in a register, no eliding, no reordering *of volatile accesses relative to each other*.
- [ ] **`volatile` — what it does not give you.**
  - [ ] Not atomicity. A `volatile uint32_t` read-modify-write is still three separate steps.
  - [ ] Not ordering with respect to *non*-volatile accesses.
  - [ ] Not a memory barrier — nothing is said to the CPU's store buffer or cache.
- [ ] **`static` at file scope** — internal linkage. The closest thing C has to a lowercase Go identifier.
- [ ] **`static` inside a function** — persistent storage, single instance, still scoped to the function. Note which section it lands in (Week 1 answer).
- [ ] **`extern`** — declaration without definition; how a global is shared across translation units.
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
