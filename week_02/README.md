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

---
<details>
<summary>Pointer arithmetics</summary>
## The core idea: pointers are typed, and arithmetic respects the type

When you write `p + 1`, C doesn't mean "the next byte." It means "the next object of whatever type `p` points to." The compiler silently multiplies by `sizeof(*p)` for you.

```c
#include <stdio.h>

int main(void) {
    int    arr[5] = {10, 20, 30, 40, 50};
    char   cbuf[5];
    double dbuf[5];

    int    *ip = arr;
    char   *cp = cbuf;
    double *dp = dbuf;

    printf("int*:    %p -> %p  (delta %td bytes)\n",
           (void*)ip, (void*)(ip + 1), (char*)(ip + 1) - (char*)ip);
    printf("char*:   %p -> %p  (delta %td bytes)\n",
           (void*)cp, (void*)(cp + 1), (char*)(cp + 1) - (char*)cp);
    printf("double*: %p -> %p  (delta %td bytes)\n",
           (void*)dp, (void*)(dp + 1), (char*)(dp + 1) - (char*)dp);
    return 0;
}
```

Typical output (x86-64):

```
int*:    0x7ffd...a10 -> 0x7ffd...a14  (delta 4 bytes)
char*:   0x7ffd...a00 -> 0x7ffd...a01  (delta 1 bytes)
double*: 0x7ffd...9c0 -> 0x7ffd...9c8  (delta 8 bytes)
```

Same source-level `+ 1`, three different byte strides.

### Why this design is right

It makes `arr[i]` and `*(arr + i)` literally the same expression. The standard *defines* subscripting that way — which is also why the cursed `3[arr]` compiles and works.

### Common bug: scaling it yourself

```c
int *p = arr;
p = p + 2 * sizeof(int);   /* WRONG: moves 8 ints = 32 bytes, way off the end */
p = p + 2;                 /* right: moves 2 ints = 8 bytes */
```

If you genuinely want byte-level movement, say so by changing the type:

```c
unsigned char *bytes = (unsigned char*)arr;
bytes += 3;                /* 3 actual bytes, into the middle of arr[0] */
```

> **Note:** `void *` arithmetic is *not* standard C. GCC/Clang allow it as an extension treating `sizeof(void)` as 1, but `-pedantic` will complain, and MSVC rejects it.

---

## Why `&arr[n] - &arr[0]` is `n`, not `n * sizeof(T)`

Subtraction is defined as the inverse of addition, so it has to un-scale. The rule in the standard is essentially: **if `q == p + n`, then `q - p == n`.** Nothing else would be self-consistent.

Concretely, the compiler emits `(byte_address_q - byte_address_p) / sizeof(*p)`:

```c
int arr[5];
int *a = &arr[0];
int *b = &arr[3];

printf("%td\n", b - a);                  /* 3  — elements */
printf("%td\n", (char*)b - (char*)a);    /* 12 — bytes    */
```

Think of it as **units cancelling**, like in physics:

| Operation | Types |
|---|---|
| Addition | `pointer + count → pointer` |
| Subtraction | `pointer − pointer → count` |

The `sizeof(T)` factor is baked into the pointer type on both operands, so it divides out. Getting `n * sizeof(T)` back would be a broken round trip: `p + (q - p)` would no longer equal `q`.

This is exactly why the idiom for array length works:

```c
#define COUNTOF(a) (sizeof(a) / sizeof((a)[0]))

int arr[5];
ptrdiff_t n = &arr[5] - &arr[0];   /* 5, not 20 */
```

### Two hard constraints

- **Both operands must point into the same array object** (or one past its end). Subtracting `&x - &y` for two unrelated variables is undefined behavior, even though it'll "work" and give you some number on flat-memory machines. The standard doesn't guarantee unrelated objects live in one comparable address space.
- **The pointed-to type must be complete.** You can't do arithmetic on `struct Foo *` if `Foo` is only forward-declared, since the compiler doesn't know the stride.

---

## `ptrdiff_t`

The result type of pointer subtraction is `ptrdiff_t`, declared in `<stddef.h>`. It is:

- **signed** (differences can be negative)
- **implementation-defined** in width — usually `long` on 64-bit Linux, `long long` on 64-bit Windows

Which is exactly why you shouldn't guess at a format specifier:

```c
#include <stddef.h>
#include <stdio.h>

ptrdiff_t d = end - begin;
printf("%td\n", d);              /* correct, portable (C99+) */
printf("%ld\n", d);              /* wrong on Windows LLP64   */
printf("%ld\n", (long)d);        /* acceptable fallback      */
```

### Signed vs. unsigned trap

The signedness matters in real code, because `sizeof` yields **unsigned** `size_t`. Mixing them invites the usual conversion trap:

```c
ptrdiff_t d = -1;
if (d < sizeof(int))                /* FALSE! d converts to a huge unsigned value */
    puts("never printed");

if (d < (ptrdiff_t)sizeof(int))     /* TRUE — compare like with like */
    puts("printed");
```

Turn on `-Wsign-compare` (included in `-Wall` for C) and the compiler will flag these.

> **Pedantic corner:** `ptrdiff_t` is only guaranteed wide enough for differences within a single array. On a 32-bit system with a `char` array larger than `PTRDIFF_MAX`, the difference technically overflows into UB. You'll basically never hit this, but it explains why the standard hedges rather than promising the type is always big enough.

---

## One-past-the-end

C explicitly blesses the address just past the last element. For `T arr[N]`, the pointer `arr + N` (equivalently `&arr[N]`) is a valid, well-defined value.

**You may:**

- **form** it
- **compare** it against other pointers into the array
- **subtract** it from them
- assign and copy it around

**You may not dereference it.** `*(arr + N)` is undefined behavior, full stop — it may read into padding, another variable, or nothing at all.

```c
int arr[5] = {1,2,3,4,5};
int *end = arr + 5;                 /* legal to form              */

for (int *p = arr; p != end; ++p)   /* legal to compare           */
    printf("%d ", *p);              /* never dereferences `end`   */

printf("%td\n", end - arr);         /* legal: 5                   */
int bad = *end;                     /* UB — don't                 */
```

This rule is what makes the entire half-open `[begin, end)` iteration style legal, which is the dominant idiom in C and the foundation of C++ iterators.

`&arr[5]` is fine *despite* looking like it subscripts out of bounds: `&*(arr + 5)` is a special case the standard carves out, since the `&` and the `*` cancel and no access occurs.

### Consequence 1 — Two past the end is UB, even without dereferencing

```c
int *p = arr + 5;   /* fine   */
int *q = arr + 6;   /* UB just to compute this value */
```

### Consequence 2 — `arr - 1` is UB too; there is no "one before the beginning"

This breaks the naive reverse loop:

```c
/* BROKEN: after the last iteration p becomes arr-1, which is UB.
   Optimizers have been known to turn this into an infinite loop. */
for (int *p = &arr[4]; p >= arr; --p)
    printf("%d ", *p);

/* Fixed: decrement inside, never step below arr */
for (int *p = arr + 5; p != arr; )
    printf("%d ", *--p);

/* Or just use an index */
for (size_t i = 5; i-- > 0; )
    printf("%d ", arr[i]);
```

### Consequence 3 — A single object counts as an array of length 1

So `&x + 1` is a legal one-past-the-end pointer:

```c
int x = 42;
int *p = &x;
int *e = &x + 1;          /* legal */
printf("%td\n", e - p);   /* 1 */
```

### Two more notes on comparison

- Pointer comparison with `<`, `>`, `<=`, `>=` is only defined within one array object (plus its one-past-end); comparing unrelated pointers that way is UB. `==` and `!=` are always fine.
- If `sizeof(T)` happens to make two different objects' addresses coincide — e.g. `&arr1[n]` equalling `&arr2[0]` when they're adjacent in memory — the comparison may compare equal, but that doesn't make the one-past pointer usable as a pointer *into* the second array.

---

## Quick summary

| Expression | Meaning / Result | Legal? |
|---|---|---|
| `p + 1` | advances by `sizeof(*p)` bytes | yes |
| `p + 2 * sizeof(T)` | overshoots by a factor of `sizeof(T)` | compiles, almost always a bug |
| `&arr[n] - &arr[0]` | `n` (element count, not bytes) | yes |
| `(char*)&arr[n] - (char*)&arr[0]` | `n * sizeof(T)` (bytes) | yes |
| type of `q - p` | `ptrdiff_t` (signed, `<stddef.h>`, print with `%td`) | — |
| `arr + N` (one past end) | form, compare, subtract | yes |
| `*(arr + N)` | dereference one-past-end | **UB** |
| `arr + N + 1` | two past end | **UB** |
| `arr - 1` | one before start | **UB** |
| `&x + 1` for scalar `x` | treated as array of length 1 | yes |
| `&a - &b` for unrelated objects | — | **UB** |
</details>
