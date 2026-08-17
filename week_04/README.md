# Week 4

## Concepts

- [x] **Set / clear / toggle / test a single bit.** `|= mask`, `&= ~mask`, `^= mask`, `& mask`.
  - [x] Note why the shift base should be `1UL` (or `1U`) and not `1`.
- [ ] **Multi-bit fields.** Mask, shift, read-modify-write. Build the field value, clear the old field, OR the new one in.
  - [ ] Mask *after* shifting so an oversized value can't spill into neighbouring fields.
- [ ] **Why read-modify-write on a hardware register is a race.** Three bus accesses; an interrupt (or another master) landing between the read and the write loses its change.
  - [ ] Atomic set/reset registers exist for exactly this. On the F3's GPIO: `BSRR` (set/reset, one write) and `BRR` (reset only).
  - [ ] Note which registers you *must* still RMW (e.g. `MODER`), and how you protect those instead.
- [ ] **Struct bitfields** — and why most embedded style guides ban them.
  - [ ] Allocation order, straddling of storage units, and padding are implementation-defined.
  - [ ] No guarantee about how many bus accesses a bitfield write becomes — fatal for hardware registers.
- [ ] **`union` type punning vs `memcpy`.** `memcpy` is the portable way; union punning is legal in C (unlike C++) but still trips alignment and strict-aliasing assumptions when pointers get involved.
  - [ ] Confirm at `-O2` that `memcpy` of 4 bytes compiles to a single load/store, i.e. it costs nothing.
- [ ] **Endianness.** Little vs big; byte swapping for protocol work.
  - [ ] Write your own `bswap16`/`bswap32` rather than relying on `htons` and friends.
  - [ ] Note why serialising byte-by-byte into a `uint8_t[]` sidesteps the whole question.
- [ ] **`make`:**
  - [ ] Targets, prerequisites, recipes; how make decides something is out of date.
  - [ ] Pattern rules (`%.o: %.c`).
  - [ ] Automatic variables: `$@`, `$<`, `$^`.
  - [ ] Variables and `:=` vs `=`.
  - [ ] Header dependency tracking with `-MMD -MP` and `-include $(DEPS)`.
  - [ ] `.PHONY` for `clean` / `all`.

## Project — `bitops.h` + a hand-written Makefile

### `bitops.h` — the macros

- [ ] Write the core macros:
  ```c
  #define BIT(n)              (1UL << (n))
  #define SET_BITS(reg, m)    ((reg) |=  (m))
  #define CLR_BITS(reg, m)    ((reg) &= ~(m))
  #define TGL_BITS(reg, m)    ((reg) ^=  (m))
  #define FIELD_SET(reg, m, sh, v) \
      ((reg) = ((reg) & ~(m)) | (((v) << (sh)) & (m)))
  ```
- [ ] Note why every parameter is parenthesised, and construct a call that breaks if you drop one.
- [ ] Note where these macros evaluate an argument more than once, and why that matters (`SET_BITS(*p++, m)`).
- [ ] Add a `FIELD_GET(reg, m, sh)` counterpart.

### The bit functions

- [ ] `uint32_t reverse_bits(uint32_t x)` — do it once with a naive loop, then again with the swap-halves/quarters/... trick. Compare both against each other in the tests.
- [ ] `int popcount(uint32_t x)` — naive loop, then the Kernighan `x &= x - 1` version, then note that Cortex-M has no popcount instruction (unlike x86's `POPCNT`).
- [ ] `int find_first_set(uint32_t x)` — define the zero case explicitly and document it. Note the relationship to `CLZ`, which the Cortex-M *does* have.
- [ ] `uint16_t rgb565_pack(uint8_t r, uint8_t g, uint8_t b)` — 5/6/5 layout; state which end red is at.
- [ ] `void rgb565_unpack(uint16_t c, uint8_t *r, uint8_t *g, uint8_t *b)` — decide whether you scale 5-bit back to 8-bit or just shift, and say why.
- [ ] `uint8_t crc8(const uint8_t *data, size_t len)` — bitwise version first; pick and record the polynomial and init value.
  - [ ] Optional: table-driven version, and note the RAM/flash trade (256 bytes of `const` table in `.rodata`).
- [ ] Build everything with the running flag set:
  ```
  -Wall -Wextra -Werror -Wconversion -std=c11
  ```
  - [ ] Expect `-Wconversion` to fight you on the RGB565 and CRC code. Fix it with explicit casts you can justify, not blanket ones.

### Tests

- [ ] `reverse_bits`: 0, `0xFFFFFFFF`, `0x00000001`, `0x80000000`, and reverse-twice-is-identity over a set of values.
- [ ] `popcount`: 0, all-ones, single bits, and the two implementations agreeing.
- [ ] `find_first_set`: 0 (documented case), bit 0, bit 31.
- [ ] RGB565: pack/unpack round trip, and the corners (black, white, pure R/G/B).
- [ ] `crc8`: a known-answer vector (e.g. the ASCII string `"123456789"`), plus empty input.
- [ ] Macro tests: `FIELD_SET` doesn't disturb bits outside its mask; an oversized value is truncated rather than spilling.

### The Makefile

- [ ] Variables: `CC`, `CFLAGS`, `SRCS`, `OBJS`, `DEPS`, `TARGET`.
- [ ] Default target `all`.
- [ ] A pattern rule compiling `%.c` → `%.o` using `$<` and `$@`.
- [ ] A link rule using `$^`.
- [ ] Add `-MMD -MP` to `CFLAGS` and `-include $(DEPS)` near the bottom.
- [ ] `.PHONY: all clean test`, and a `clean` that removes objects, deps, and the binary.
- [ ] A `test` target that builds and runs the test binary.
- [ ] No CMake this week — you need to have felt this before you let a tool do it.

### Prove the incremental build

- [ ] `make` from clean — everything compiles.
- [ ] `make` again — nothing compiles (`make: Nothing to be done`).
- [ ] `touch` one `.c` — exactly one object rebuilds, then relink.
- [ ] `touch bitops.h` — every object that includes it rebuilds, and nothing that doesn't.
- [ ] Delete the `.d` files and repeat the header test — watch it silently fail to rebuild. That's what `-MMD` is for.
- [ ] Record the before/after so you can explain it later.

## Done when

- [ ] Touching a header rebuilds exactly the right objects and nothing else.
- [ ] Every bit function passes its tests, including the edge cases you had to define yourself.
- [ ] You can read your Makefile top to bottom and explain every line without guessing.
- [ ] You can explain why RMW on a GPIO register is a race, and what `BSRR` does about it.

<details>
<summary>Single-Bit Operations</summary>

## The mental model

An integer is a row of bits. Bit 0 is the rightmost (least significant) bit.

```
uint8_t x = 0b01001010;   // value 74

 bit index:  7  6  5  4  3  2  1  0
 bit value:  0  1  0  0  1  0  1  0
```

To work on one bit, you build a **mask**. A mask is a value with a `1` in the
position you want, and `0` everywhere else.

```c
#define BIT(n)  (1UL << (n))

BIT(0) ==  0b00000001
BIT(3) ==  0b00001000
BIT(5) ==  0b00100000
```

The shift `1UL << n` moves the single `1` bit `n` places to the left.

Then you apply one bitwise operator. Each operator does a different job.

## The four operations

| Goal | Code | Operator | Result |
|---|---|---|---|
| **Set** bit `n` to 1 | `x \|= BIT(n)` | OR | bit becomes 1, others unchanged |
| **Clear** bit `n` to 0 | `x &= ~BIT(n)` | AND with inverted mask | bit becomes 0, others unchanged |
| **Toggle** bit `n` | `x ^= BIT(n)` | XOR | bit flips, others unchanged |
| **Test** bit `n` | `x & BIT(n)` | AND | non-zero if the bit is 1 |

The truth tables show why each operator works.

```
 OR (|)          AND (&)         XOR (^)
 0 | 0 = 0       0 & 0 = 0       0 ^ 0 = 0
 0 | 1 = 1       0 & 1 = 0       0 ^ 1 = 1
 1 | 0 = 1       1 & 0 = 0       1 ^ 0 = 1
 1 | 1 = 1       1 & 1 = 1       1 ^ 1 = 0
```

Read the tables with the mask bit as the second operand:

- `OR` with `1` always gives 1 → **forces the bit on**.
- `OR` with `0` keeps the original → **other bits are safe**.
- `AND` with `0` always gives 0 → **forces the bit off**.
- `AND` with `1` keeps the original → **other bits are safe**.
- `XOR` with `1` inverts the original → **flips the bit**.
- `XOR` with `0` keeps the original → **other bits are safe**.

## Set a bit: `x |= mask`

```c
uint8_t x = 0b01001010;
x |= (1U << 5);          // set bit 5

//   0100 1010   x
// | 0010 0000   mask = 1 << 5
//   ---------
//   0110 1010   result
```

Set is **idempotent**. If the bit is already 1, nothing changes.

## Clear a bit: `x &= ~mask`

You need a mask that is `0` in the target position and `1` everywhere else.
Build the normal mask first, then invert it with `~`.

```c
uint8_t x = 0b01001010;
x &= ~(1U << 3);         // clear bit 3

//   1 << 3   =  0000 1000
//  ~(1 << 3) =  1111 0111    <- inverted mask

//   0100 1010   x
// & 1111 0111   ~mask
//   ---------
//   0100 0010   result
```

Clear is also idempotent. If the bit is already 0, nothing changes.

> The `~` operator is where most type bugs happen.

## Toggle a bit: `x ^= mask`

```c
uint8_t x = 0b01001010;
x ^= (1U << 1);          // toggle bit 1

//   0100 1010   x
// ^ 0000 0010   mask
//   ---------
//   0100 1000   result (bit 1 went 1 -> 0)

x ^= (1U << 1);          // toggle again
//   0100 1010   back to the original value
```

Toggle is its own inverse. Two toggles return the original value.

## Test a bit: `x & mask`

```c
uint8_t x = 0b01001010;

if (x & (1U << 3)) {
    // true: bit 3 is set
}
```

**Important:** the result is `mask`, not `1`.

```c
x & (1U << 3)   ==  8      // not 1
x & (1U << 6)   ==  64     // not 1
x & (1U << 5)   ==  0      // bit is clear
```

This is fine inside `if`, because C treats any non-zero value as true.
But do not compare the result to `1`:

```c
if ((x & (1U << 3)) == 1)   // WRONG: only ever true for bit 0
```

To get a clean `0` or `1`, use one of these forms:

```c
int bit = !!(x & (1UL << n));    // logical-not twice
int bit = (x >> n) & 1U;         // shift the bit down instead
```

### Precedence trap

The `==` and `!=` operators bind **tighter** than `&`, `|` and `^`.

```c
if (x & MASK == 0)     // parses as  x & (MASK == 0)   -> almost always wrong
if ((x & MASK) == 0)   // correct
```

Always put parentheses around a bitwise expression before you compare it.

## Why `1UL` (or `1U`) and not `1`?

The literal `1` has type `int`. On most platforms, `int` is **32-bit and
signed**. That causes three separate problems.

### Problem 1: shifting into the sign bit is undefined behaviour

For a signed left operand, the C standard requires the result to be
representable in the result type. Bit 31 of a 32-bit `int` is the sign bit.

```c
1 << 31     // undefined behaviour (signed overflow)
1U << 31    // well defined: 0x80000000
```

In practice, compilers usually produce the "expected" bits. But the compiler
is allowed to assume that undefined behaviour never happens. Optimisers use
that assumption, and the code can break at high optimisation levels.

Unsigned types have no sign bit and no overflow. The standard defines their
shifts and their wrap-around fully.

### Problem 2: the shift count must be smaller than the type width

A shift count that is equal to or larger than the width of the (promoted) left
operand is undefined behaviour. The width of the **left** operand decides the
limit, not the width of the destination.

```c
uint64_t x = 0;

x |= (1 << 40);      // UB: left operand is a 32-bit int
x |= (1ULL << 40);   // correct: left operand is 64-bit
```

Assigning to a 64-bit variable does not widen the shift. The shift is
evaluated first, in the type of `1`.

### Problem 3: `~mask` truncates the upper bits (the sneaky one)

This bug only appears in the **clear** operation, and only with wide types.

```c
uint64_t flags = 0xFFFFFFFFFFFFFFFF;

flags &= ~(1U << 3);        // BUG
```

Step by step:

1. `1U << 3` is `8`, with type `unsigned int` (32-bit).
2. `~8U` is `0xFFFFFFF7`, still 32-bit `unsigned int`.
3. The conversion to `uint64_t` **zero-extends**: `0x00000000FFFFFFF7`.
4. The AND clears bit 3 **and all of bits 32 to 63**.

Result: `0x00000000FFFFFFF7` instead of `0xFFFFFFFFFFFFFFF7`.

With a 64-bit literal, the mask is inverted at the correct width:

```c
flags &= ~(1ULL << 3);      // 0xFFFFFFFFFFFFFFF7  -> correct
```

> **A curiosity that makes this bug hard to find.** The signed version
> `~(1 << 3)` gives `-9` as an `int`. Conversion of a signed value to a wider
> unsigned type **sign-extends**, so it becomes `0xFFFFFFFFFFFFFFF7` — which
> is accidentally correct. So `1` appears to work, `1U` visibly breaks, and
> only `1ULL` is correct for the right reason. Do not rely on the accident:
> it still leaves you with the undefined behaviour of problems 1 and 2.

### Which suffix to use

Match the literal to the width of the variable you modify.

| Variable type | Use |
|---|---|
| `uint8_t`, `uint16_t`, `uint32_t`, `unsigned int` | `1U` |
| `unsigned long` | `1UL` |
| `uint64_t`, `unsigned long long` | `1ULL` or `UINT64_C(1)` |

Be careful with `1UL`. The width of `unsigned long` is platform-dependent:

- 64-bit Linux and macOS (LP64): `unsigned long` is **64-bit**.
- 64-bit Windows (LLP64): `unsigned long` is **32-bit**.
- 32-bit platforms: `unsigned long` is **32-bit**.

So `1UL` is not portable for bits 32 and higher. For fixed-width work, use
`1ULL`, or `UINT64_C(1)` from `<stdint.h>`, which is always correct.

### One more note: right shift

The `>>` operator on a **negative signed** value is implementation-defined.
Almost all compilers do an arithmetic shift and copy the sign bit in from the
left. That fills your result with `1` bits.

```c
int32_t  a = -8;
uint32_t b = (uint32_t)-8;

a >> 1;   // 0xFFFFFFFC  (sign bit copied in)
b >> 1;   // 0x7FFFFFFE  (zero shifted in)
```

This is another reason to keep bit-manipulation values unsigned.

## Reusable macros

```c
#include <stdint.h>

#define BIT(n)              (UINT64_C(1) << (n))

#define BIT_SET(x, n)       ((x) |=  BIT(n))
#define BIT_CLEAR(x, n)     ((x) &= ~BIT(n))
#define BIT_TOGGLE(x, n)    ((x) ^=  BIT(n))
#define BIT_TEST(x, n)      (((x) & BIT(n)) != 0)

/* Write a computed value into a bit, with no branch. */
#define BIT_WRITE(x, n, v)  ((v) ? BIT_SET(x, n) : BIT_CLEAR(x, n))
```

Note the parentheses around every macro parameter. They protect against
precedence surprises when a caller passes an expression such as `i + 1`.

An inline function is safer than a macro, because it evaluates each argument
exactly once and it checks types:

```c
static inline uint64_t bit_set(uint64_t x, unsigned n)    { return x |  (UINT64_C(1) << n); }
static inline uint64_t bit_clear(uint64_t x, unsigned n)  { return x & ~(UINT64_C(1) << n); }
static inline uint64_t bit_toggle(uint64_t x, unsigned n) { return x ^  (UINT64_C(1) << n); }
static inline bool     bit_test(uint64_t x, unsigned n)   { return (x >> n) & 1U; }
```

## Multi-bit masks

The same four operators work on masks with several bits.

```c
#define FLAGS_ALL  (FLAG_READY | FLAG_ERROR | FLAG_BUSY)

status |=  FLAGS_ALL;      // set all three
status &= ~FLAGS_ALL;      // clear all three
status ^=  FLAGS_ALL;      // toggle all three
```

For a test with a multi-bit mask, decide which question you ask:

```c
if (status & FLAGS_ALL)                  // ANY of the bits is set
if ((status & FLAGS_ALL) == FLAGS_ALL)   // ALL of the bits are set
if ((status & FLAGS_ALL) == 0)           // NONE of the bits is set
```

### Extract a bit field

To read `w` bits that start at position `n`:

```c
uint32_t field = (x >> n) & ((1U << w) - 1);
```

`(1U << w) - 1` produces `w` low `1` bits. For example `(1U << 3) - 1` is
`0b111`.

To write a field, clear it first, then OR the new value in:

```c
uint32_t mask = ((1U << w) - 1) << n;
x = (x & ~mask) | ((value << n) & mask);
```

## Checklist

- Use unsigned literals: `1U`, `1UL`, `1ULL`, or `UINT64_C(1)`.
- Match the literal width to the variable width, above all for `~`.
- Keep the shift count below the width of the left operand.
- Put parentheses around a bitwise expression before you compare it.
- Do not compare a test result to `1`. Use `!= 0` or `!!`.
- Use named flag constants, not magic numbers.
- Use `<stdint.h>` fixed-width types (`uint32_t`, `uint64_t`) for hardware
  registers and wire formats.
</details>
