# Week 2

## Concepts

- [x] **Pointer arithmetic.** `p + 1` advances by `sizeof(*p)`, not by 1 byte. Know why `&arr[n] - &arr[0]` is `n` and not `n * sizeof(T)`.
  - [x] Pointer difference has type `ptrdiff_t`.
  - [x] One-past-the-end is legal to *form* and compare, illegal to dereference.
- [x] **Arrays are not pointers, but decay to them.** `sizeof` on an array gives the array size; on a decayed parameter it gives the pointer size.
  - [x] `void f(int a[10])` is really `void f(int *a)` — the `10` is documentation, nothing more.
- [x] **`void*` for generic code.** Implicit conversion to and from any object pointer; cannot be dereferenced or arithmetic'd (portably).
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
The core idea: pointers are typed, and arithmetic respects the type

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
| Addition | `pointer + integer → pointer` |
| Subtraction | `pointer − pointer → integer` |

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
<details>
<summary>Arrays Are Not Pointers (But They Decay Into Them)</summary>

## An array is not a pointer — it's a block

```c
int arr[10];   // 40 bytes of ints. Type: "array of 10 int"
int *p;        // 8 bytes holding an address. Type: "pointer to int"
```

When you write `int arr[10];`, the compiler sets aside 40 bytes (assuming 4-byte ints) and gives the name `arr` the type *array of 10 int*. **There is no pointer variable anywhere.** Nothing in memory holds the address of the first element — the address is simply where the block happens to live.

Contrast with `int *p;`, which allocates one pointer-sized object (8 bytes on a typical 64-bit machine) whose *contents* are an address.

So `int[10]` and `int *` are genuinely different types, with different sizes and different memory layouts.

```
arr:  [ i0 ][ i1 ][ i2 ] ... [ i9 ]        40 bytes, no indirection
p:    [ 0x7ffd...      ]  ──────────►      8 bytes, points elsewhere
```

## Decay: the implicit conversion

The confusion exists because of one rule: **almost anywhere an array appears in an expression, it is implicitly converted to a pointer to its first element.** That conversion is called *decay*.

```c
int arr[10];
int *p = arr;        // arr decays to &arr[0]
foo(arr);            // decays
arr[3];              // really *(arr + 3), so arr decays here too
```

### Where decay does *not* happen

| Context | Example | Result |
|---|---|---|
| Operand of `sizeof` | `sizeof arr` | `40`, not `8` |
| Operand of `_Alignof` | `_Alignof(int[10])` | alignment of `int` |
| Operand of unary `&` | `&arr` | type `int (*)[10]` |
| String literal init'ing a char array | `char s[] = "hi";` | copies characters, no pointer stored |
| `typeof` (C23) | `typeof(arr)` | `int[10]` |

That short exception list is the whole story. Everything else about `sizeof` and function parameters follows from it.

## `sizeof` — array vs. decayed pointer

Because `sizeof` is one of the exceptions, it sees the *real* array type:

```c
int arr[10];
sizeof arr;      // 40  — the whole block
sizeof arr[0];   // 4   — one element

int *p = arr;
sizeof p;        // 8   — the size of the ADDRESS, not the target
sizeof *p;       // 4   — one element
```

Hence the classic length idiom, which **works only on a real array**:

```c
size_t n = sizeof arr / sizeof arr[0];   // 10
```

### The `&arr` subtlety

```c
&arr[0]   // type int *          — arithmetic steps 4 bytes
&arr      // type int (*)[10]    — arithmetic steps 40 bytes
```

Both hold the same numeric address. Their *types* differ, so pointer arithmetic on them advances by different amounts.

## The function parameter rule

C has a separate rule for parameters: **any parameter declared as "array of T" is adjusted by the compiler to "pointer to T".** This isn't decay at the call site — it's a rewrite of the declaration itself.

All three declare *exactly the same function*:

```c
void f(int a[10]);
void f(int a[]);
void f(int *a);
```

The `10` is discarded entirely:

- it is **not** checked against the caller
- it does **not** affect `sizeof a` inside the body (which gives `8`)
- passing an array of 3 elements is legal as far as the type system is concerned

> **The `10` is documentation for human readers. Nothing more.**

This is also why **arrays cannot be passed by value in C**: there is no way to write a parameter whose type is an array. The workaround is to wrap the array in a `struct`, which *is* copied by value.

## 5. The trap

```c
void f(int a[10]) {
    size_t n = sizeof a / sizeof a[0];   // 8 / 4 == 2.  WRONG, silently.
}
```

No warning by default on many compilers. The idiom that works outside the function fails inside it, because `a` is a pointer here.

### Fix 1 — pass the length (conventional)

```c
void f(int *a, size_t n);

f(arr, sizeof arr / sizeof arr[0]);   // compute length where arr is still an array
```

### Fix 2 — pointer to array (strict; keeps size in the type)

```c
void f(int (*a)[10]) {
    size_t n = sizeof *a / sizeof (*a)[0];   // 10, correct
    (*a)[0] = 1;                             // or a[0][0]
}

f(&arr);    // caller must pass &arr
            // wrong-sized arrays are now a compile-time type error
```

### Fix 3 — `static` in the parameter (C99, advisory)

```c
void f(int a[static 10]);   // "caller promises at least 10 elements"
```

Still a pointer parameter, still `sizeof a == 8`. But compilers may use it for warnings and optimization, and it documents a contract the compiler partly understands.

## Multidimensional arrays

Decay peels off **only the outermost dimension**.

`int m[3][4]` is *an array of 3 things, each of which is `int[4]`*. So it decays to `int (*)[4]` — **not** `int **`. There is no array of pointers involved anywhere; the 12 ints are one contiguous block.

```c
void g(int m[3][4]);      // becomes  void g(int (*m)[4]);
```

- The leading `3` is dropped, as always.
- The `4` is **load-bearing** — it's part of the pointer's target type, and the compiler needs it to compute the address of `m[i][j]`. Drop it and the code won't compile.

```c
int m[3][4];
sizeof m;        // 48  — whole block
sizeof m[0];     // 16  — one row
sizeof m[0][0];  // 4   — one element
m;               // decays to int (*)[4]
```

## Summary

| Thing | Type | `sizeof` |
|---|---|---|
| `int arr[10]` | `int[10]` | 40 |
| `arr` in most expressions | `int *` (decayed) | — |
| `&arr` | `int (*)[10]` | 8 |
| `int *p` | `int *` | 8 |
| `int a[10]` as a parameter | `int *` | 8 |
| `int m[3][4]` | `int[3][4]` | 48 |
| `m` in most expressions | `int (*)[4]` | — |

**Rules of thumb**

1. Compute array lengths only where the array is still an array — never inside a function that received it as a parameter.
2. If a function takes an array, it takes a pointer. Pass the length alongside it, or take `T (*)[N]` if the size is genuinely fixed.
3. `int **` is never the decayed form of a 2D array.
</details>

<details>
<summary>void * for generic code</summary>

## What `void *` actually is

`void` is an *incomplete type that can never be completed*. It has no size and no values. `void *` is a pointer to that nothing.

This is not a trick or a special case bolted on — the two properties that make `void *` useful and the two that make it awkward both fall out of that one fact:

| Because `void` has no size... | Consequence |
|---|---|
| there's no representation to commit to | it can hold *any* object address |
| there's nothing to produce when you follow it | you can't dereference it |
| there's no stride to step by | you can't do arithmetic on it |
| there's nothing to measure | `sizeof(void)` is invalid |

`void *` is an address with the type information deliberately stripped off.

## Implicit conversion, both directions

In C — unlike C++ — `void *` converts implicitly to and from any **object** pointer type. No cast needed, in either direction.

```c
int   i = 42;
void *v = &i;          // int*  -> void*   implicit, fine
int  *p = v;           // void* -> int*    implicit, ALSO fine in C

char *buf = malloc(n); // void* -> char*   this is why malloc "just works"
```

### Don't cast `malloc`

```c
int *p = malloc(n * sizeof *p);          // idiomatic C
int *p = (int *)malloc(n * sizeof *p);   // noise; C++ habit
```

The cast is not merely redundant. Historically, if you forgot `#include <stdlib.h>`, the cast would silence the diagnostic about the implicitly-declared `malloc` returning `int`, turning a caught bug into a silent one. In C99 and later, implicit declarations are gone, but the habit is still worth keeping.

### The round-trip guarantee

Converting `T * → void * → T *` gives you back a pointer that compares equal to the original and is usable. The standard guarantees this. So `void *` is a safe *transport* type: you can hand a pointer through a generic layer and get it back intact.

What you get back is only guaranteed good if you convert it back to **the type it came from**. `void *` doesn't remember; you have to.

## What `void *` does *not* cover

### Function pointers

```c
void (*fn)(void);
void *v = fn;         // NOT guaranteed by the C standard
```

Object pointers and function pointers are separate universes in C. The standard says nothing about converting between them, and on exotic architectures (Harvard-style, segmented, wide code pointers) they genuinely differ.

POSIX requires the conversion to work, because `dlsym` returns `void *` and there'd be no way to use it otherwise. That's the origin of this notorious workaround:

```c
void (*fn)(void);
*(void **)&fn = dlsym(handle, "my_function");   // POSIX-blessed ugliness
```

If you're writing portable C, keep function pointers in function pointer types. Cast between *different* function pointer types if you must (that round-trips fine), but don't route them through `void *`.

### `const` qualification

`void *` loses const, so the conversion that would discard it is not implicit:

```c
const int *ci = &i;
const void *cv = ci;    // fine
void *v = cv;           // error: discards const, needs a cast
```

Generic read-only APIs take `const void *` (see `memcmp`, `qsort`'s comparator). Generic write APIs take `void *`.

### `void **` is not generic

`void *` is a generic pointer *to an object*. `void **` is a specific pointer to a `void *` object — it is **not** a generic pointer-to-pointer.

```c
int *p;
void **pp = &p;    // error: int** and void** are not compatible
```

This trips people writing "generic allocator" APIs of the shape `int alloc_thing(void **out)`. Callers must pass a real `void *` variable, or you accept the cast and the formal aliasing violation that goes with it.

## No dereference, no arithmetic

```c
void *v = buf;

*v;        // error: dereferencing pointer to incomplete type
v[3];      // error: same thing, v[3] is *(v + 3)
v + 1;     // error in standard C: how many bytes is "1"?
v - w;     // error: same problem
```

The fix is to say what stride you mean:

```c
unsigned char *b = v;
b + 1;                          // unambiguously 1 byte
(unsigned char *)v + offset;    // one-off
```

### The GCC/Clang extension

GCC and Clang treat `void *` arithmetic as `char *` arithmetic — stride 1 — as a documented extension. It compiles silently by default.

```
-Wpointer-arith    warn about it
-pedantic          diagnose it
```

Relying on it is a portability hazard, and it's genuinely ambiguous to readers who don't know which dialect they're in. Cast explicitly.

### Alignment: the quiet trap

`void *` carries no alignment information. Converting a `void *` back to `T *` when the address isn't suitably aligned for `T` is **undefined behaviour** — not merely slow, and not merely a problem on old hardware (it breaks SSE/NEON loads on current CPUs).

```c
unsigned char raw[64];
uint32_t *p = (uint32_t *)(raw + 1);   // misaligned. UB on dereference.
```

Generic code that carves objects out of a byte buffer has to do the alignment arithmetic itself. `_Alignof` / `alignof`, `_Alignas` / `alignas`, and `aligned_alloc` are the tools. Memory from `malloc` is aligned for any object type with a fundamental alignment, so it's the easy path.

## `void *` in practice

### Callbacks with a context pointer

The standard shape for "call my function later and give it back my data":

```c
void for_each(int *a, size_t n, void (*fn)(int, void *), void *ctx)
{
    for (size_t i = 0; i < n; i++)
        fn(a[i], ctx);
}

struct total { long sum; };

static void add(int x, void *ctx)
{
    struct total *t = ctx;      // implicit conversion back
    t->sum += x;
}

struct total t = {0};
for_each(arr, n, add, &t);
```

`for_each` never knows what `ctx` is. It only promises to pass it through untouched. That's the whole idea.

### `qsort` — and its classic bug

```c
int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a;      // pointers TO elements, not the elements
    int y = *(const int *)b;
    return (x > y) - (x < y);      // no subtraction: avoids overflow
}

qsort(arr, n, sizeof arr[0], cmp_int);
```

Two things bite people here:

1. `a` and `b` point *at* the elements. For an array of `int *`, the comparator receives `int **`.
2. `return x - y;` overflows for large-magnitude inputs and silently mis-sorts. Use the comparison form above.

### A generic container

The pattern is: store the element size, and move elements with `memcpy`.

```c
struct vec {
    unsigned char *data;    // char-family, so arithmetic is legal
    size_t elem_size, len, cap;
};

void *vec_at(struct vec *v, size_t i)
{
    return v->data + i * v->elem_size;
}

void vec_push(struct vec *v, const void *elem)
{
    /* grow if needed ... */
    memcpy(v->data + v->len++ * v->elem_size, elem, v->elem_size);
}
```

Note `data` is `unsigned char *`, not `void *` — precisely so the arithmetic is portable. The `void *` appears at the API boundary, where the genericity is wanted.

### The cost

`void *` deletes the compiler's ability to check you. Pass the wrong element size, cast to the wrong type, mismatch a callback signature, and you get no diagnostic — just corruption at runtime. Every `void *` in an interface is a small hand-written contract that the compiler will not enforce. Keep them at boundaries and convert back to real types immediately.

## Summary

| | |
|---|---|
| Converts implicitly to/from any **object** pointer | yes, both directions, in C |
| Converts to/from **function** pointers | not per the standard; POSIX requires it |
| `const void *` → `void *` | needs a cast |
| `void **` as a generic pointer-to-pointer | no such thing |
| `*v`, `v + 1`, `v - w`, `sizeof(void)` | all invalid; GCC/Clang allow arithmetic as an extension |
| Alignment | not tracked; misaligned conversion back is UB |
| Type safety | none — the compiler cannot help you |

### Rules of thumb

1. Let `void *` exist only at API boundaries; convert back to a real type on the first line of the callee.
</details>

<details>
<summary>char * aliasing</summary>

## The strict aliasing rule, briefly

C restricts which *lvalue type* you may use to access a given object. Roughly (C11 6.5p7), you may access an object through:

- its own type, or a qualified version of it (`const`/`volatile`)
- the signed or unsigned counterpart of its type
- an aggregate or union type containing one of the above
- **a character type**

Anything else is undefined behaviour. This isn't pedantry for its own sake — it's what lets the optimizer assume that a write through an `int *` cannot disturb a `float` it has cached in a register.

The last bullet is the escape hatch.

## The exception: any object may be read as bytes

```c
double d = 3.14;
unsigned char *b = (unsigned char *)&d;

for (size_t i = 0; i < sizeof d; i++)
    printf("%02x ", b[i]);      // completely legal
```

This works for *any* object of *any* type. It is the licence that makes `memcpy`, `memcmp`, `memset`, hashing, serialization, checksums, and hex dumps expressible in C at all.

### Prefer `unsigned char`

All three character types are on the allowed list, but `unsigned char` is the right choice for byte inspection:

- **Plain `char` may be signed.** Byte values above 127 become negative. That breaks comparisons, right-shifts (implementation-defined for negative values), and array indexing (`table[c]` with negative `c` reads out of bounds — a real bug in naïve `<ctype.h>` and UTF-8 code).
- **`unsigned char` has no padding bits and no trap representations.** Every bit pattern is a valid value, so reading raw bytes through it can never produce something the implementation considers invalid.

Use `signed char` when you mean "small signed integer", `char` when you mean "text character", and `unsigned char` when you mean "byte". They are three distinct types, and `char` is not a typedef for either of the others.

### A note on `uint8_t`

In practice `uint8_t` is a typedef for `unsigned char`, and therefore inherits the exception. But the standard does not *require* that — it could in principle be an extended integer type, which would not be a character type and would not get the licence. On any platform you're likely to meet, this is a non-issue; in maximally portable code, `unsigned char` is the type with the guarantee attached.

## Why the reverse does not hold

The rule is written in terms of *the object being accessed* and *the lvalue used to access it*. "Character type" is on the list of permitted lvalues **for every object**. But no corresponding entry makes arbitrary types permitted lvalues **for a character object**.

So this is fine:

```c
struct point pt = { 1, 2 };
unsigned char *b = (unsigned char *)&pt;    // OK: reading a struct as bytes
```

...and this is not:

```c
unsigned char buf[sizeof(struct point)];
struct point *p = (struct point *)buf;      // formally UB
p->x = 1;                                   // accessing char objects via struct lvalue
```

Two separate defects in the second version:

1. **Aliasing.** The declared type of `buf` is `unsigned char[N]`; `struct point` is not an allowed access type for it.
2. **Alignment.** `unsigned char[N]` has alignment 1. The address may not satisfy `_Alignof(struct point)`, which is UB independently of aliasing.

The asymmetry is deliberate. Reading bytes out of a typed object is an inspection that can't confuse the optimizer's model. Imposing a rich type onto storage the compiler believes is bytes can, and does.

## The `malloc` distinction

Here's the part that reconciles §8 with everyday C, where casting allocated memory to a struct pointer is completely normal.

An object declared with a type has that as its **effective type**, permanently. Memory returned by `malloc` has **no declared type**. Its effective type is whatever type was used for the most recent store into it.

```c
struct point *p = malloc(sizeof *p);   // no declared type
p->x = 1;                              // effective type becomes struct point. Fine.
```

That is why:

| Storage | Reinterpret as `struct T`? |
|---|---|
| `malloc(sizeof(struct T))` | Yes — allocated memory has no declared type, and `malloc` returns suitably aligned memory |
| `unsigned char buf[sizeof(struct T)]` | No — declared type is `unsigned char[]`; also alignment 1 |
| `union { struct T t; unsigned char b[sizeof(struct T)]; } u` | Yes — see below |

If you need a typed view of a stack buffer, the buffer should be declared as a union, or declared as the target type in the first place.

## Type punning, correctly

### Use `memcpy`

The portable, always-correct way to reinterpret a value's bits:

```c
float f = 1.0f;
uint32_t bits;
memcpy(&bits, &f, sizeof bits);      // 0x3f800000
```

This is legal because `memcpy` operates on bytes, and both directions of byte access are permitted. It reads `f` as bytes (allowed by the exception) and writes into `bits`, whose declared type then governs how you read it.

**This is not slow.** Every mainstream compiler recognizes a small fixed-size `memcpy` and emits a single load/store, or nothing at all if the value is already in the right register class. `memcpy` is the idiomatic type-pun in modern C, not a fallback.

The thing it replaces:

```c
uint32_t bits = *(uint32_t *)&f;     // strict-aliasing violation
```

This is not a theoretical concern. At `-O2`, GCC and Clang will reorder or elide stores around such a cast and produce wrong output. It's one of the most common sources of "works at `-O0`, breaks at `-O2`".

### Or use a union

Unlike C++, C explicitly permits reading a union member other than the one last written; the bytes are reinterpreted in the new member's type.

```c
union fbits { float f; uint32_t u; };

union fbits x = { .f = 1.0f };
printf("%08x\n", x.u);               // well-defined in C
```

The union also solves the alignment problem, which makes it the tool for typed views of stack storage:

```c
union buf {
    struct point p;
    unsigned char bytes[sizeof(struct point)];
};

union buf b;
recv_bytes(b.bytes, sizeof b.bytes);
use(&b.p);                           // aligned, and legal
```

The caveats are that the result may be a trap representation for some types, and that reading a member wider than the one written exposes unspecified bytes.

## Padding, and why `memcmp` on structs lies

Structs may contain padding bytes between members and at the end. Their contents are **unspecified** — copying, assigning, or partially initializing a struct says nothing about what lands in the padding.

```c
struct s { char c; int i; };         // typically 3 bytes of padding after c

struct s a = { 'x', 1 };
struct s b;
b.c = 'x';
b.i = 1;

memcmp(&a, &b, sizeof a);            // may be nonzero! padding differs
```

So:

- **Never** use `memcmp` to test structs for equality. Compare members.
- **Never** hash a struct by hashing its bytes, unless you've cleared it with `memset` first *and* you're certain nothing has re-touched the padding.
- **Never** serialize a struct by writing its bytes to a file or socket. Padding, endianness, and member sizes are all implementation-dependent. Serialize field by field.

Reading padding bytes is legal (they're just bytes); *relying* on their values is not.

## `-fno-strict-aliasing`

GCC and Clang accept `-fno-strict-aliasing`, which tells the optimizer to assume any pointer may alias any other. The Linux kernel builds with it, as do several older codebases with too much punning to audit.

It is a real, useful escape valve for legacy code, and it costs some optimization. But it is a *compiler* promise, not a *language* one — code that depends on it is not portable C, and the next compiler or the next `-O` level may not oblige. For new code, `memcpy` costs nothing and needs no flag.

## Summary

| | |
|---|---|
| Read/write any object's bytes via `char`/`signed char`/`unsigned char` | legal, always |
| Read a `char` array as some other type | UB (aliasing) plus likely UB (alignment) |
| `malloc`'d memory as any type | legal — no declared type, suitably aligned |
| Best type for byte access | `unsigned char` |
| Portable type punning | `memcpy` — free at `-O2` |
| Union punning | legal in C (unlike C++) |
| `memcmp` / hashing / serializing structs by bytes | broken: padding is unspecified |

### Rules of thumb

1. For byte arithmetic, hold the pointer as `unsigned char *`, not `void *`.
2. To reinterpret bits, reach for `memcpy` first and a union second. Never `*(T *)&x`.
3. To get typed storage, use `malloc` or a union — not a cast onto a `char` array.
4. Compare and serialize structs field by field.
</details>
