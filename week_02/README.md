# Week 2

## Concepts

- [ ] **Pointer arithmetic.** `p + 1` advances by `sizeof(*p)`, not by 1 byte. Know why `&arr[n] - &arr[0]` is `n` and not `n * sizeof(T)`.
  - [ ] Pointer difference has type `ptrdiff_t`.
  - [ ] One-past-the-end is legal to *form* and compare, illegal to dereference.
- [ ] **Arrays are not pointers, but decay to them.** `sizeof` on an array gives the array size; on a decayed parameter it gives the pointer size.
  - [ ] `void f(int a[10])` is really `void f(int *a)` — the `10` is documentation, nothing more.
  - [ ] Write out explicitly what a Go slice carries that a `T*` does not (length, capacity, bounds checks, GC-tracked ownership).
- [ ] **`void*` for generic code.** Implicit conversion to and from any object pointer; cannot be dereferenced or arithmetic'd (portably).
- [ ] **`char*` aliasing.** Any object may legally be inspected byte-by-byte through `char*`/`unsigned char*`; the reverse is not generally true.
- [ ] **Function pointers.**
  - [ ] Declaration syntax: `int (*fp)(void *, uint8_t *, size_t);`
  - [ ] Arrays of function pointers (dispatch tables).
  - [ ] `typedef`'d function-pointer types — use these, they are how you'll build every interface for the rest of the roadmap.
- [ ] **`const` placement.** `const char *p` (pointee is const) vs `char * const p` (pointer is const) vs `const char * const p`.
  - [ ] Read declarations right-to-left and be able to say each one out loud.
- [ ] **Struct pointers.** `->` vs `(*p).`, passing structs by pointer instead of by value.
- [ ] **Designated initialisers.** `.field = value`, and why they're safer than positional initialisers when a struct changes.

## Project A — Ring buffer, no allocation

Caller supplies storage; the module never allocates.

### Interface

- [ ] Write `ringbuf.h`:
  ```c
  typedef struct {
      uint8_t  *buf;
      size_t    capacity;
      size_t    head, tail;
      bool      full;
  } ringbuf_t;

  void   rb_init (ringbuf_t *rb, uint8_t *storage, size_t capacity);
  bool   rb_put  (ringbuf_t *rb, uint8_t byte);
  bool   rb_get  (ringbuf_t *rb, uint8_t *out);
  size_t rb_count(const ringbuf_t *rb);
  ```
- [ ] Note why `rb_count` takes `const ringbuf_t *` and the others don't.
- [ ] Note why the caller supplies `storage` instead of the module calling `malloc`.

### Implementation

- [ ] `rb_init` — store the pointer, set `head = tail = 0`, `full = false`. Don't touch the caller's bytes.
- [ ] `rb_put` — return `false` when full (do not overwrite).
- [ ] `rb_get` — return `false` when empty; write through `out` only on success.
- [ ] Handle the **full/empty ambiguity**: `head == tail` means both. Resolve it with the `full` flag and say in a comment what the alternative (sacrifice one slot) would have cost.
- [ ] Handle **wraparound** — index advance via `(i + 1) % capacity`.
  - [ ] Note what changes if you require `capacity` to be a power of two and mask instead of modulo. (This matters later on an MCU.)

### Tests

- [ ] Put then get one byte.
- [ ] Fill to capacity, confirm the next `rb_put` returns `false`.
- [ ] Drain to empty, confirm the next `rb_get` returns `false`.
- [ ] Fill, drain, fill again — proves `head`/`tail` wrap correctly rather than only working once.
- [ ] Interleaved put/get across the wrap point.
- [ ] `rb_count` correct at empty, partial, full, and post-wrap.
- [ ] Capacity 1 edge case.
- [ ] Build with the Week 1 flags:
  ```
  gcc -Wall -Wextra -Werror -Wconversion -std=c11 ringbuf.c test_ringbuf.c -o test_rb
  ```

## Project B — An interface, by hand

Build the C equivalent of a Go interface and feel the difference.

### Interface

- [ ] Write `io_stream.h`:
  ```c
  typedef struct {
      int  (*read) (void *ctx, uint8_t *buf, size_t len);
      int  (*write)(void *ctx, const uint8_t *buf, size_t len);
      void  *ctx;
  } io_stream_t;
  ```
- [ ] Note that this is a hand-rolled vtable + receiver: the function pointers are the method set, `ctx` is the receiver.

### Backends

- [ ] Backend 1: **memory buffer** — reads and writes against a caller-supplied `uint8_t[]`.
- [ ] Backend 2: **stdout** — `write` goes to `fwrite`, `read` returns an error or 0.
- [ ] Each backend has its own `ctx` struct and a constructor returning a filled-in `io_stream_t`.

### Consumer

- [ ] Write one function that takes `io_stream_t *` and works without knowing which backend it got (e.g. `stream_write_all` or a hexdump-to-stream).
- [ ] Run it against both backends from `main`.

### The comparison

- [ ] In comments, list every piece of safety Go's interface gave you that this does **not**:
  - [ ] Compile-time check that a type actually implements the method set.
  - [ ] Type safety on the receiver — `ctx` is `void*`, nothing stops you pairing the wrong ctx with the wrong function table.
  - [ ] Nil-method protection — a `NULL` function pointer is a crash, not a compile error.
  - [ ] Lifetime guarantees — nothing keeps `ctx` alive as long as the stream.
  - [ ] Method sets attached to the type rather than assembled by hand at every construction site.
- [ ] Note which of these you could recover with discipline (a `const` static vtable, an init function per backend) and which you simply cannot.

## Done when

- [ ] The ring buffer handles wraparound and the full/empty ambiguity, with tests proving it — not just tests that pass.
- [ ] You can write a function-pointer declaration and a `const`-qualified pointer declaration from memory, correctly, first try.
- [ ] You can explain out loud what a Go slice carries that a `T*` does not.
- [ ] Both files are kept: the ring buffer returns in **Week 15** (UART) and **Week 21** (event queue); `io_stream_t` returns in **Week 16** (testable drivers).
