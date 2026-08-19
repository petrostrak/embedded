# Week 4

## Concepts

- [x] **Set / clear / toggle / test a single bit.** `|= mask`, `&= ~mask`, `^= mask`, `& mask`.
  - [x] Note why the shift base should be `1UL` (or `1U`) and not `1`.
- [x] **Multi-bit fields.** Mask, shift, read-modify-write. Build the field value, clear the old field, OR the new one in.
  - [x] Mask *after* shifting so an oversized value can't spill into neighbouring fields.
- [x] **Why read-modify-write on a hardware register is a race.** Three bus accesses; an interrupt (or another master) landing between the read and the write loses its change.
  - [x] Atomic set/reset registers exist for exactly this. On the F3's GPIO: `BSRR` (set/reset, one write) and `BRR` (reset only).
  - [x] Note which registers you *must* still RMW (e.g. `MODER`), and how you protect those instead.
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

<details>
<summary>Multi-bit fields</summary>

A single bit is a flag. A **multi-bit field** is a group of adjacent bits that hold
one number. Example: bits 4 to 11 of a 32-bit register hold a prescaler value from
0 to 255.

Two numbers define a field:

| Property | Meaning |
|---|---|
| **Shift** | The position of the lowest bit of the field. |
| **Width** | The number of bits in the field. |

From these two numbers you make the **mask**: a value with a 1 in every bit that
belongs to the field, and a 0 everywhere else.

```c
/* width ones, moved into position */
mask = ((1u << width) - 1u) << shift;
```

Step by step, for width = 8 and shift = 4:

```
1u << 8            = 0x00000100
0x100 - 1          = 0x000000FF   /* 8 ones at the bottom */
0xFF << 4          = 0x00000FF0   /* the ones moved into position */
```

**Practical advice:** write the mask as a hex constant in your header. It is easier
to read, and the compiler does no work at run time.

```c
#define TIMER_PRESCALER_MASK  0x00000FF0u   /* bits 4-11 */
```

## The Example Register

The examples below use this 32-bit control register.

```
 31            28 27                    16 15  14 13 12 11           4 3  2 1  0
+----------------+------------------------+------+-----+--------------+--+--+----+
|    reserved    |         RELOAD         | rsvd |CLKSR|  PRESCALER   |IE|EN|MODE|
+----------------+------------------------+------+-----+--------------+--+--+----+
```


| Field | Bits | Shift | Width | Binary (low 16 bits) | Mask |
|---|---|---|---|---|---|
| MODE | 0–1 | 0 | 2 | `0000 0000 0000 0011` | `0x00000003` |
| ENABLE | 2 | 2 | 1 | `0000 0000 0000 0100` | `0x00000004` |
| IRQ_EN | 3 | 3 | 1 | `0000 0000 0000 1000` | `0x00000008` |
| PRESCALER | 4–11 | 4 | 8 | `0000 1111 1111 0000` | `0x00000FF0` |
| CLKSRC | 12–13 | 12 | 2 | `0011 0000 0000 0000` | `0x00003000` |
| RELOAD | 16–27 | 16 | 12 | (upper half) `0000 1111 1111 1111` | `0x0FFF0000` |

```c
#include <stdint.h>

#define TIMER_MODE_MASK        0x00000003u   /* bits 0-1,   width 2  */
#define TIMER_ENABLE_MASK      0x00000004u   /* bit  2,     width 1  */
#define TIMER_IRQ_EN_MASK      0x00000008u   /* bit  3,     width 1  */
#define TIMER_PRESCALER_MASK   0x00000FF0u   /* bits 4-11,  width 8  */
#define TIMER_CLKSRC_MASK      0x00003000u   /* bits 12-13, width 2  */
#define TIMER_RELOAD_MASK      0x0FFF0000u   /* bits 16-27, width 12 */
```

## Read a Field

Move the field down to bit 0, then remove everything else.

```c
uint32_t prescaler = (reg >> 4) & 0xFFu;
```

Shift first, then mask. If you mask first you must use the positioned mask, which
gives the same answer with more steps.

```c
/* generic form */
#define FIELD_READ(reg, shift, width) \
    (((uint32_t)(reg) >> (shift)) & ((1u << (width)) - 1u))
```

## Write a Field — Read, Modify, Write

You cannot write a field on its own. A field is part of a word, so you must write
the whole word. Therefore you do three things:

1. **Read** the current word.
2. **Modify** your copy: clear the old field, then OR in the new field.
3. **Write** the whole word back.

```c
uint32_t tmp = reg;                          /* 1. read              */
tmp &= ~TIMER_PRESCALER_MASK;                /* 2a. clear old field  */
tmp |= (new_value << 4) & TIMER_PRESCALER_MASK;  /* 2b. OR in new field */
reg = tmp;                                   /* 3. write             */
```

The `&= ~mask` step is what protects the other fields. Every bit outside the mask
keeps its old value, including reserved bits you must not disturb.

**Do not skip the clear.** `reg |= value << 4` can only set bits to 1. It can never
return a bit to 0, so you cannot lower the prescaler with it.

## Why You Mask *After* the Shift

The `& mask` is what makes the OR safe. It restricts the OR to exactly the bits the clear step prepared.
Same code, but suppose `new_value` is 0x155 — nine bits, one too many:

```
new_value << 4       0001 0101 0101 0000    0x00001550
                        ^ bit 12: belongs to CLKSRC
TIMER_PRESCALER_MASK 0000 1111 1111 0000    0x00000FF0
AND                  ---------------------
result               0000 0101 0101 0000    0x00000550
                        ^ removed
```

### What it is not
It is not validation. It does not reject the bad value or tell you about it. It contains the blast radius and no more. 
That is why section 9 pairs it with an assertion — the mask keeps the bug local, the assert makes it visible.

**Summary:** the mask does not make a bad value good. It stops a bad value from
spreading.

### One mask, two jobs

Note that the same constant does the clear (`& ~mask`) and the limit (`& mask`).
This is the reason to prefer masking after the shift instead of masking the raw
value with a separate width mask. One constant cannot get out of step with itself.

## Reusable Macros

### Version A — explicit shift and width

Clear to read, but you must keep two numbers correct for each field.

```c
#define FIELD_MASK(shift, width) \
    (((1u << (width)) - 1u) << (shift))

#define FIELD_GET(reg, shift, width) \
    (((uint32_t)(reg) >> (shift)) & ((1u << (width)) - 1u))

#define FIELD_PREP(shift, width, val) \
    (((uint32_t)(val) << (shift)) & FIELD_MASK((shift), (width)))

#define FIELD_MODIFY(reg, shift, width, val) \
    ((reg) = ((uint32_t)(reg) & ~FIELD_MASK((shift), (width))) \
             | FIELD_PREP((shift), (width), (val)))
```

### Version B — mask only (recommended)

The mask alone already contains the shift and the width. You can extract the shift
from the mask, so the mask becomes the single source of truth. This is the style the
Linux kernel uses.

```c
/* isolates the lowest set bit: 0x00000FF0 -> 0x00000010 */
#define LOWBIT(mask)  ((uint32_t)(mask) & (~(uint32_t)(mask) + 1u))

/* multiply by the lowest bit == shift left; divide == shift right */
#define FIELD_GET(mask, reg) \
    (((uint32_t)(reg) & (uint32_t)(mask)) / LOWBIT(mask))

#define FIELD_PREP(mask, val) \
    (((uint32_t)(val) * LOWBIT(mask)) & (uint32_t)(mask))

#define FIELD_MODIFY(reg, mask, val) \
    ((reg) = ((uint32_t)(reg) & ~(uint32_t)(mask)) | FIELD_PREP((mask), (val)))
```

The multiply and divide look expensive. They are not. For a constant mask the
compiler turns them into a shift. The advantage is that this works in plain C,
with no compiler extension such as `__builtin_ctz`.

Usage:

```c
uint32_t reg = 0x00A02001u;

FIELD_MODIFY(reg, TIMER_PRESCALER_MASK, 42u);
FIELD_MODIFY(reg, TIMER_CLKSRC_MASK, 1u);

uint32_t p = FIELD_GET(TIMER_PRESCALER_MASK, reg);   /* 42 */
```

## Hardware Registers

For a memory-mapped register you must add `volatile`. Without it the compiler can
remove or reorder your accesses.

```c
static inline void reg_modify(volatile uint32_t *reg,
                              uint32_t mask,
                              uint32_t val)
{
    uint32_t tmp = *reg;                        /* read   */
    tmp &= ~mask;                               /* clear  */
    tmp |= (val * LOWBIT(mask)) & mask;         /* insert */
    *reg = tmp;                                 /* write  */
}
```

Keep the read in a local variable. Do not write `*reg = (*reg & ~mask) | ...`,
because that reads the hardware twice.

### Write all fields in one operation

Each read-modify-write is one read and one write on the bus. If you must set five
fields, build the word first and write it once.

```c
uint32_t cfg = FIELD_PREP(TIMER_MODE_MASK,      2u)
             | FIELD_PREP(TIMER_PRESCALER_MASK, 42u)
             | FIELD_PREP(TIMER_CLKSRC_MASK,    1u)
             | FIELD_PREP(TIMER_RELOAD_MASK,    1000u)
             | TIMER_IRQ_EN_MASK;

*TIMER_CTRL = cfg;      /* one write, no intermediate states */
```

Each `FIELD_PREP` produces a word that is empty except for its own field. OR then merges them, 
because no two of them have a 1 in the same bit position.

```
FIELD_PREP(TIMER_MODE_MASK,      2u)   ->  0x00000002
FIELD_PREP(TIMER_PRESCALER_MASK, 42u)  ->  0x000002A0
FIELD_PREP(TIMER_CLKSRC_MASK,    1u)   ->  0x00001000
FIELD_PREP(TIMER_RELOAD_MASK,    1000u)->  0x03E80000
TIMER_IRQ_EN_MASK                      ->  0x00000008


MODE        0000 0000 0000 0000 0000 0000 0000 0010  (2)
PRESCALER   0000 0000 0000 0000 0000 0010 1010 0000  (42)      
CLKSRC      0000 0000 0000 0000 0001 0000 0000 0000  (1)
RELOAD      0000 0011 1110 1000 0000 0000 0000 0000  (1000)
IRQ_EN      0000 0000 0000 0000 0000 0000 0000 1000  (1)
            ------------------- OR -------------------
cfg         0000 0011 1110 1000 0001 0010 1010 1010    = 0x03E812AA
```

This also avoids illegal intermediate states. Some hardware reacts to every write.

## Catch Oversized Values Early

The mask contains the damage. It does not report the bug. Add a check.

Compile time, for constant values:

```c
#define FIELD_FITS(mask, val) \
    ((((uint32_t)(val) * LOWBIT(mask)) & ~(uint32_t)(mask)) == 0u)

_Static_assert(FIELD_FITS(TIMER_PRESCALER_MASK, 42u), "prescaler too large");
```

Run time, for computed values:

```c
assert(FIELD_FITS(TIMER_PRESCALER_MASK, prescaler));
```
</details>

<details>
<summary>Read-Modify-Write Races on Hardware Registers (STM32F3, C)</summary>

## What a register access is in C

A peripheral register is a fixed address in the memory map. CMSIS gives you a
`volatile` struct pointer:

```c
#include "stm32f3xx.h"      /* GPIOA, RCC, TIM2 ... */

/* GPIOA is:  ((GPIO_TypeDef *) 0x48000000UL)
   Each member of GPIO_TypeDef is declared __IO, which is volatile uint32_t. */
```

`volatile` tells the compiler two things:

* Do not cache the value in a CPU register.
* Do not remove, merge, or reorder the accesses.

`volatile` does **not** make an access atomic. This is the single most common
misunderstanding. It controls *how many* accesses happen, not whether they
happen as one indivisible step.

## Why `|=` is three bus accesses

```c
GPIOA->ODR |= (1U << 5);        /* set PA5 high */
```

The Cortex-M4 has no "OR into memory" instruction. The compiler must emit:

```asm
LDR  r1, [r0]        ; 1. READ   ODR from the bus
ORR  r1, r1, #0x20   ; 2. MODIFY in a CPU register
STR  r1, [r0]        ; 3. WRITE  ODR back to the bus
```

Between step 1 and step 3 the old value of every other bit in that register is
sitting in a CPU register. It is a stale snapshot. If anything changes the real
register during that window, step 3 overwrites the change.

The same applies to `&=`, `^=`, `++`, and any bitfield assignment.

### The failure timeline

Main code drives PA5. An interrupt handler drives PA9. Both use `|=` on `ODR`.

An interrupt handler is a function that you never call. You register it, and the hardware calls it for you when an event happens (timer tick, UART byte, pin edge). The CPU stops the main code between two machine instructions, saves the registers, runs the handler, restores the registers, and resumes the main code at the exact instruction where it stopped. The main code does not know it happened.

| Time | Main loop | Interrupt handler | ODR in hardware |
|---|---|---|---|
| t0 | | | `0x0000` |
| t1 | `LDR` → r1 = `0x0000` | | `0x0000` |
| t2 | *IRQ taken* | `LDR` → `0x0000` | `0x0000` |
| t3 | | `ORR` + `STR` `0x0200` | `0x0200` (PA9 high) |
| t4 | | *return* | `0x0200` |
| t5 | `ORR` r1 = `0x0020` | | `0x0200` |
| t6 | `STR` `0x0020` | | `0x0020` (**PA9 lost**) |

Nobody cleared PA9 because nobody had to. STR is not "set bit 5" — it is "make ODR equal to r1". r1 was 0x0020, so every other bit became 0 as a side effect. The main loop wrote a value that was correct at t1 and wrong at t6.

### It is not only interrupts

Anything else that can touch the same register is another bus master:

* **DMA** writing a peripheral register while the CPU does RMW on it.
* **The peripheral itself.** Hardware sets status flags at any moment.
* **A debugger** writing through the AHB debug port.
* **Another core**, on parts that have one (not the F3, but keep the habit).
* **A second RTOS task** that is preempted mid-sequence. Same race, no
  interrupt involved.

## The atomic way: `BSRR` and `BRR`

STM32 GPIO ports give you write-only registers that need **no read**. One `STR`
on the bus. There is no window to interrupt, because there is nothing between
the read and the write — there is no read.

### `GPIOx_BSRR` — 32 bits, set and reset

| Bits | Name | Effect when you write 1 |
|---|---|---|
| 15:0 | `BS0..BS15` | **Set** the matching pin |
| 31:16 | `BR0..BR15` | **Reset** the matching pin |

Writing 0 to a bit does nothing. Untouched pins are untouched. You never need
to know the current state of the port.

```c
#define LED_PIN   5U

/* Set PA5 — one bus write, atomic */
GPIOA->BSRR = (1U << LED_PIN);

/* Clear PA5 — high half of BSRR */
GPIOA->BSRR = (1U << (LED_PIN + 16U));

/* Or use the dedicated reset register (F3 has both) */
GPIOA->BRR  = (1U << LED_PIN);
```

Set and reset in the same write is allowed. If you set both `BSx` and `BRx` for
the same pin, the **set wins** (`BSx` has priority). Do not rely on that; it
usually means your mask is wrong.

```c
/* Drive a 4-bit bus on PA0..PA3 to value `v`, one atomic write.
   Clear all four, set the ones we want. */
static inline void bus_write(uint32_t v)
{
    GPIOA->BSRR = ((0x0FU & ~v) << 16U) | (v & 0x0FU);
}
```

### `GPIOx_BRR` — 16 bits, reset only

Convenience register. `GPIOA->BRR = mask;` equals writing `mask << 16` to
`BSRR`. Same atomicity.

### Read/write summary

| Register | Access | Use |
|---|---|---|
| `IDR` | read-only | read pin input state |
| `ODR` | read/write | read the current output latch |
| `BSRR` | **write-only** | set and/or reset pins atomically |
| `BRR` | **write-only** | reset pins atomically |

Reading `BSRR` returns `0x00000000`. Never do `GPIOA->BSRR |= x;` — this is
still an RMW, it reads garbage, and it defeats the whole purpose. Always plain
assignment `=`.

### A clean tiny driver

```c
static inline void pin_high(GPIO_TypeDef *port, uint32_t pin)
{
    port->BSRR = (1U << pin);
}

static inline void pin_low(GPIO_TypeDef *port, uint32_t pin)
{
    port->BSRR = (1U << (pin + 16U));
}

static inline void pin_write(GPIO_TypeDef *port, uint32_t pin, bool level)
{
    port->BSRR = level ? (1U << pin) : (1U << (pin + 16U));
}
```

### The toggle trap

The F3 GPIO has no atomic toggle register. Toggle needs the old state:

```c
GPIOA->ODR ^= (1U << 5);                          /* RMW — races */

uint32_t odr = GPIOA->ODR;                        /* also races: read-then-write */
GPIOA->BSRR = ((odr & (1U<<5)) << 16) | (~odr & (1U<<5));
```

The second form makes the *write* atomic, but the read and the write are still
two steps. It is safer than `^=` because only your own pin is affected — other
pins are never clobbered. But your own pin can still be wrong if another
context also drives that pin. If two contexts must toggle the same pin, keep a
software shadow variable and protect that, or give the pin one owner.

## Registers you must still read-modify-write

Atomic set/reset exists only for GPIO output data. Everything that packs
multi-bit fields, or that has no shadow register, needs RMW.

**GPIO configuration**

| Register | Why RMW | Bits per pin |
|---|---|---|
| `MODER` | mode field, no atomic alias | 2 |
| `OSPEEDR` | speed field | 2 |
| `PUPDR` | pull-up/pull-down field | 2 |
| `AFR[0]`, `AFR[1]` | alternate function selection | 4 |
| `OTYPER` | 1 bit, but no set/reset register | 1 |
| `LCKR` | key sequence | — |

```c
/* PA5 as output, push-pull, low speed, no pull */
GPIOA->MODER  &= ~(3U << (5 * 2));
GPIOA->MODER  |=  (1U << (5 * 2));       /* 01 = output */
GPIOA->OTYPER &= ~(1U << 5);
GPIOA->PUPDR  &= ~(3U << (5 * 2));
```

Note that `MODER` is **per port, not per pin**. Two drivers that own different
pins on the same port still write the same register. This is where the race
usually appears in real projects.

**Other common RMW registers**

* `RCC->AHBENR`, `RCC->APB1ENR`, `RCC->APB2ENR` — clock enables. Two drivers
  enabling their clock at the same time is the textbook lost-update bug.
* Peripheral control registers: `TIM2->CR1`, `USART1->CR1`, `SPI1->CR1`,
  `I2C1->CR1`, `ADC1->CFGR`, `DMA1_Channel1->CCR`.
* `EXTI->IMR`, `EXTI->RTSR` — masks and edge selection.

**Registers that look like RMW but are not**

Some registers are write-1-to-clear. On those, `|=` and `&= ~` are both wrong,
and a plain write is both correct *and* atomic.

```c
/* WRONG: read-modify-write on a w1c register.
   Any flag that hardware sets between the read and the write is lost. */
EXTI->PR |= EXTI_PR_PR0;

/* RIGHT: single write, clears only PR0, atomic */
EXTI->PR = EXTI_PR_PR0;

/* USART ICR is also write-1-to-clear */
USART1->ICR = USART_ICR_ORECF;
```

Timer status registers on the F3 are read-clear-write-0 (`rc_w0`). Clearing one
flag with `&=` reads the register and writes back zeros for every flag that was
not set at read time — so a flag raised in that window is silently lost. Write
ones everywhere except the flag you clear:

```c
TIM2->SR &= ~TIM_SR_UIF;    /* risky: can drop a flag set mid-sequence */
TIM2->SR  = ~TIM_SR_UIF;    /* safer: single write, clears only UIF      */
```

**NVIC is already atomic.** `ISER`, `ICER`, `ISPR`, `ICPR` are write-1-to-act
registers. `NVIC_EnableIRQ()` does a plain write. No protection needed.

## How to protect the RMW registers

Pick the cheapest option that works. In order of preference:

### 1 Configure once, before concurrency exists

Most RMW registers are configuration. Touch them in `SystemInit` / board init,
while interrupts are still disabled and before the scheduler starts. Then never
touch them again. No lock needed, zero cost. This solves 90% of real cases.

```c
int main(void)
{
    /* interrupts still masked here */
    clocks_init();
    gpio_init();            /* all MODER / PUPDR / AFR writes live here */
    peripherals_init();

    __enable_irq();         /* only now can anything else run */
    for (;;) { app_step(); }
}
```

### 2 Single-owner discipline

Give each register exactly one writing context, and document it. If only the
`TIM2_IRQHandler` ever writes `TIM2->CR1`, there is no race. Write the owner in
a comment next to the declaration. Reviewers can then check it.

This does not work for shared registers such as `MODER` or `RCC->APBxENR`,
because ownership is per-register, not per-bit.

### 3 Critical section: mask interrupts

Save and restore `PRIMASK`. Never call bare `__disable_irq()` /
`__enable_irq()` in a function that may itself be called from inside another
critical section — the inner `__enable_irq()` would re-enable too early.

```c
#include "cmsis_compiler.h"

static inline uint32_t critical_enter(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DMB();                    /* order accesses across the boundary */
    return primask;
}

static inline void critical_exit(uint32_t primask)
{
    __DMB();
    __set_PRIMASK(primask);     /* restores, does not blindly enable */
}

void gpio_set_mode(GPIO_TypeDef *port, uint32_t pin, uint32_t mode)
{
    uint32_t s = critical_enter();
    port->MODER &= ~(3U << (pin * 2U));
    port->MODER |=  (mode << (pin * 2U));
    critical_exit(s);
}
```

Keep the section as short as possible. Every cycle inside it is added interrupt
latency for the whole system. No loops, no waiting on hardware, no `printf`.

### 4 `BASEPRI` instead of `PRIMASK`

`PRIMASK` blocks *everything*. If you have hard real-time handlers (motor
commutation, a timer capture), raise `BASEPRI` instead. Interrupts with a
priority number below the threshold keep running.

```c
static inline uint32_t critical_enter_basepri(uint32_t new_basepri)
{
    uint32_t old = __get_BASEPRI();
    __set_BASEPRI(new_basepri << (8U - __NVIC_PRIO_BITS));
    __DMB();
    return old;
}
```

Remember that `BASEPRI` cannot mask NMI or HardFault. Also remember that lower
numbers mean higher priority.

### 5 Bit-banding — not available for F3 GPIO

Bit-banding maps a single bit to its own word address, so a bit write becomes
one bus write. The Cortex-M3/M4 peripheral bit-band alias covers only
`0x40000000 – 0x400FFFFF`.

On the F3, GPIO sits on AHB2 at `0x48000000`, **outside** the bit-band region.
So you cannot bit-band GPIO on the F3, unlike on the F1. Bit-banding still
works for peripherals inside the APB/AHB1 range. Code that assumes it works
everywhere is a portability bug.

### 6 `LDREX` / `STREX`

The Cortex-M4 has exclusive access instructions. `STREX` fails if anything
touched the address since the `LDREX`, so you retry.

```c
static void reg_set_bits(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t v;
    do {
        v = __LDREXW((volatile uint32_t *)reg) | mask;
    } while (__STREXW(v, (volatile uint32_t *)reg) != 0U);
}
```

Use with care. Arm does not guarantee exclusive accesses to Device or
Strongly-ordered memory; behaviour on peripheral addresses is
implementation-defined. Also, an ISR that clears the exclusive monitor turns
this into a livelock risk if the retry loop is unbounded. For peripheral
registers on the F3, prefer 5.1–5.4. `LDREX`/`STREX` is the right tool for
shared variables in RAM.

### 7 RTOS mutex — task level only

A `SemaphoreHandle_t` or `osMutex` protects task-against-task. It does **not**
protect against an ISR, because an ISR cannot block. If both a task and a
handler touch the register, you still need interrupt masking (or the FreeRTOS
`taskENTER_CRITICAL_FROM_ISR` pair).

## Decision table

| You want to | Use | Atomic |
|---|---|---|
| Drive an output pin high / low | `BSRR` / `BRR` | yes |
| Drive several pins to a pattern | one `BSRR` write | yes |
| Toggle a pin | shadow variable + `BSRR` | write only |
| Read pin state | `IDR` | yes (single read) |
| Clear a w1c flag (`EXTI->PR`, `USART->ICR`) | plain `=` with one bit | yes |
| Enable an interrupt in the NVIC | `NVIC_EnableIRQ` | yes |
| Change pin mode / speed / pull / AF | RMW + init-time or critical section | no |
| Enable a peripheral clock | RMW + init-time or critical section | no |
| Change a peripheral `CRx` bit at runtime | RMW + critical section | no |
</details>

<details>
<summary>Struct bitfields</summary>

A bitfield is a struct member with a declared width in bits.

```c
struct flags {
    unsigned int ready   : 1;
    unsigned int error   : 1;
    unsigned int channel : 4;
    unsigned int mode    : 2;
};
```

The intent looks clear: pack 8 logical fields into one byte, and read or write them by name.

```c
struct flags f = {0};
f.ready = 1;
f.channel = 7;
```

The compiler generates the shift and mask operations for you. That is the only promise. Everything about the *placement* of those bits is either implementation-defined or unspecified.

## What the standard actually guarantees

Very little. The list below is short on purpose.

| Item | Status |
| --- | --- |
| The bits you asked for exist and hold the range you asked for | Guaranteed |
| Fields are allocated in an "addressable storage unit" | Guaranteed, but the unit size is implementation-defined |
| A zero-width unnamed field starts a new storage unit | Guaranteed |
| Order of bits inside a unit (low-to-high or high-to-low) | **Implementation-defined** |
| Whether a field may straddle a storage unit boundary | **Implementation-defined** |
| Size and alignment of the storage unit | **Implementation-defined** |
| Amount and position of padding | **Implementation-defined** |
| Value of padding bits | **Unspecified** |
| Signedness of a plain `int` bitfield | **Implementation-defined** |
| Types other than `int`, `signed int`, `unsigned int`, `_Bool` | **Implementation-defined** |
| Number of memory accesses one field write becomes | **No guarantee at all** |

Bitfields also have hard language limits:

- You cannot apply `&` to a bitfield. There is no address.
- You cannot apply `sizeof` to a bitfield.
- You cannot make an array of bitfields.
- You cannot use a bitfield with `offsetof`.
- A pointer to a bitfield does not exist, so no generic accessor functions.

## Allocation order — the first trap

The standard does not say which end of the storage unit the first field occupies.

```c
struct ctrl {
    unsigned int a : 4;   /* declared first */
    unsigned int b : 4;
};
```

Two compilers, both correct:

```
Compiler X (little-endian bit order, e.g. GCC on ARM/x86):
  bit:  7 6 5 4 3 2 1 0
        b b b b a a a a      -> a is in the low nibble

Compiler Y (big-endian bit order, e.g. some big-endian ABIs, IBM XL):
  bit:  7 6 5 4 3 2 1 0
        a a a a b b b b      -> a is in the high nibble
```

Set `a = 3` and `b = 0`. One compiler writes `0x03`. The other writes `0x30`.

### Consequences

- The layout is not portable between architectures.
- The layout can change between compilers on the *same* architecture.
- The layout can change between ABI versions of the same compiler.
- Bit order and byte order are separate decisions. Knowing the endianness of the CPU does not tell you the bit order of your compiler.

So a struct with bitfields is not a valid description of a wire protocol, a file format, or a hardware register. It is a private detail of one build.

## Straddling and padding — the second trap

Ask for a field that does not fit in the remaining space of the current unit. The compiler picks one of two legal behaviours.

```c
struct pkt {
    unsigned int a : 6;
    unsigned int b : 6;
};
```

If the storage unit is 8 bits:

```
Option 1 — straddle allowed (12 bits used, 4 bits padding at the end):
  byte0: a a a a a a b b
  byte1: b b b b . . . .

Option 2 — no straddle (a is padded to the byte end, b starts fresh):
  byte0: a a a a a a . .
  byte1: b b b b b b . .
```

`sizeof(struct pkt)` is 2 in both cases here, but `b` is in a different place. With more fields, the total size also changes.

### Related effects

**`sizeof` is not predictable.** This struct is often 4 bytes, but 2 bytes is legal, and 8 is legal on a machine with 64-bit storage units:

```c
struct s { unsigned int x : 1; };
```

**Padding bits are garbage.** They are unspecified, even after an initializer sets every named field. This breaks three common operations:

- `memcmp(&a, &b, sizeof a)` can report a difference when all named fields are equal.
- Hashing or checksumming the struct bytes gives unstable results.
- Writing the struct to a file or a socket leaks whatever was in that memory.

Use field-by-field comparison instead. Never compare or serialize bitfield structs as raw bytes.

**Signedness surprises.** A plain `int` bitfield may be signed or unsigned:

```c
struct t { int x : 3; };   /* range is 0..7, or -4..3 — implementation-defined */

struct t v;
v.x = 5;
printf("%d\n", v.x);       /* prints 5, or -3 */
```

Always write `unsigned int` or `signed int` explicitly. A single-bit `signed` field can hold only 0 and -1, which is almost never what the author wanted.

## The hardware register problem — the reason for the ban

This is the failure that destroys devices, so it deserves its own section.

### The tempting code

```c
/* DO NOT DO THIS */
typedef struct {
    volatile unsigned int enable    : 1;
    volatile unsigned int irq_clear : 1;
    volatile unsigned int reserved  : 6;
    volatile unsigned int prescale  : 8;
    volatile unsigned int status    : 16;
} timer_ctrl_t;

#define TIMER ((timer_ctrl_t *)0x40001000)

TIMER->enable = 1;
```

It reads well. It is also unsafe for at least five separate reasons.

### 1. A write is a read-modify-write

The CPU cannot write one bit. To set `enable`, the compiler must load the register, modify a bit, and store it back.

```
LDR  r0, [r1]        ; read the whole register
ORR  r0, r0, #1      ; set bit 0
STR  r0, [r1]        ; write the whole register
```

Now consider a status register where bit 5 is *write-1-to-clear*. The read returns 1 for a pending interrupt. The store writes that 1 back, and clears an interrupt you never handled. Your bit assignment silently acknowledged an unrelated event.

Any register with write-1-to-clear bits, read-only bits with side effects, or a FIFO data port is corrupted by a read-modify-write.

### 2. The access width is the compiler's choice

Nothing in the standard fixes the width of the load and the store. A compiler may legally implement `TIMER->prescale = 4;` as:

- one 32-bit load and one 32-bit store, or
- one 8-bit load and one 8-bit store on the third byte, or
- two 16-bit accesses, or
- a bit-set instruction on a bit-band alias.

Many peripherals accept only 32-bit accesses. A byte-wide store to such a register raises a bus fault, or is ignored, or writes the wrong sub-word. The peripheral datasheet demands a specific width. The C source cannot demand it.

The same problem breaks memory-mapped access over a bridge, and any register where a write to the low half must land before the high half.

### 3. The number of accesses is the compiler's choice

Two field writes may become two read-modify-write pairs, or one merged store, or something in between.

```c
TIMER->prescale = 4;
TIMER->enable   = 1;
```

If the compiler merges these into one store, the hardware never sees the intermediate state. If the sequence "set prescale, *then* enable" matters — and in hardware it usually does — the behaviour changes. If the compiler splits them, you get two glitch states on the bus.

`volatile` limits some of this, but it does not fix it. `volatile` says the accesses must occur and must not be reordered relative to each other. It does not say how wide each access is, or how many accesses one field write becomes. On top of that, `volatile` on bitfields is a known weak point in compiler implementations, and MISRA C treats volatile bitfields as a defect.

### 4. Adjacent fields are one memory location

For C11 concurrency purposes, all bitfields in one storage unit are a **single memory location**. Two separate non-bitfield members can be written by two threads safely. Two bitfields in the same unit cannot.

```c
/* Thread A */ ctrl.enable = 1;
/* Thread B */ ctrl.prescale = 8;
```

This is a data race. Each thread reads the whole unit and writes it back. One update is lost. The same applies to an ISR and main code, and to two CPU cores. No amount of `volatile` prevents it. You need a lock, or an atomic read-modify-write, or a hardware set/clear register.

## The correct pattern for registers

Use a whole-word `volatile` object plus explicit masks and shifts. The access width, the access count, and the bit positions are then all visible in the source.

```c
#include <stdint.h>

#define TIMER_BASE   0x40001000u
#define TIMER_CTRL   (*(volatile uint32_t *)(TIMER_BASE + 0x00u))
#define TIMER_STATUS (*(volatile uint32_t *)(TIMER_BASE + 0x04u))

/* Bit definitions taken directly from the datasheet. */
#define CTRL_ENABLE_Msk     (1u << 0)
#define CTRL_IRQCLEAR_Msk   (1u << 1)
#define CTRL_PRESCALE_Pos   8u
#define CTRL_PRESCALE_Msk   (0xFFu << CTRL_PRESCALE_Pos)

static inline void timer_set_prescale(uint32_t value)
{
    uint32_t reg = TIMER_CTRL;                 /* one 32-bit read  */
    reg &= ~CTRL_PRESCALE_Msk;
    reg |= (value << CTRL_PRESCALE_Pos) & CTRL_PRESCALE_Msk;
    TIMER_CTRL = reg;                          /* one 32-bit write */
}

/* Write-1-to-clear: write only the bit, never read first. */
static inline void timer_clear_irq(void)
{
    TIMER_STATUS = CTRL_IRQCLEAR_Msk;
}
```

Properties of this version:

- Exactly one read and one write, both 32 bits wide, on every target.
- Bit positions are constants that you can check against the datasheet.
- The write-1-to-clear case skips the read, which a bitfield cannot express.
- The accessor is an ordinary function, so you can unit-test it and mock the register.

For protocol and file formats, do the same: read the bytes into a `uint8_t` array, then extract fields with shifts and masks. That code is portable, and it survives a compiler change.

For a "set one bit atomically" need, prefer the hardware feature: a separate SET register, a separate CLEAR register, or a bit-band alias. If none exists, use an atomic operation or disable interrupts around the read-modify-write.

## When bitfields are acceptable

Bitfields are not forbidden by the language, and they are not always wrong. They are safe when **no external party depends on the layout**.

Acceptable:

- Compact internal state or flag sets inside one program, built by one compiler.
- Large in-memory tables where the memory saving is measured and real.
- Code that never compares, hashes, copies as bytes, or transmits the struct.

Not acceptable:

- Hardware registers.
- Network packets, bus frames, on-disk formats.
- Any structure shared across an ABI boundary, a language boundary, or a process boundary.
- Any structure written by more than one thread or by an ISR.

If you do use them, apply these rules:

1. Declare every field `unsigned int` or `signed int`, never plain `int` and never a narrow type.
2. Never use a single-bit signed field.
3. Never rely on `sizeof`, on field order, or on the bytes of the struct.
4. Never mark a bitfield `volatile`.
5. Add a static assert on the struct size, so a toolchain change fails the build instead of failing in the field.

```c
_Static_assert(sizeof(struct flags) == 4, "unexpected bitfield layout");
```

## What the style guides say

The rules below are the usual reason a reviewer rejects bitfields. Check the current text of each standard before you cite it in a document.

- **MISRA C:2012 Rule 6.1** — a bitfield must have an appropriate type. In C90 that means `unsigned int` or `signed int` only.
- **MISRA C:2012 Rule 6.2** — a single-bit named bitfield must not be signed.
- **MISRA C:2012 Directive 1.1** — you must identify and document every use of implementation-defined behaviour. Bitfield layout is implementation-defined, so each use needs a documented justification.
- **CERT C INT12-C** — do not assume the type of a plain `int` bitfield in an expression.
- **CERT C CON32-C** — prevent data races when threads access bitfields.
- **Barr Group Embedded C Coding Standard** — bitfields must not be used to access peripheral registers; use masks and shifts.
- **Linux kernel style** — bitfields are acceptable for internal flags, but not for anything that describes hardware or a wire format.

The pattern is consistent. The guides rarely ban the feature outright. They ban it for the cases where layout matters, which in embedded work is most cases.

## Summary

| Question | Answer |
| --- | --- |
| Which bit does my first field use? | Implementation-defined. |
| Can a field cross a unit boundary? | Implementation-defined. |
| How big is the struct? | Implementation-defined. |
| What is in the padding? | Unspecified. Do not read it. |
| Is `int x : 3` signed? | Implementation-defined. |
| How wide is the bus access for a write? | Not specified. The compiler decides. |
| How many bus accesses does one write take? | Not specified. The compiler decides. |
| Are two adjacent fields thread-safe? | No. They are one memory location. |
| Does `volatile` fix any of this? | No. It fixes elision and reordering only. |

**One sentence:** bitfields give you readable syntax over an unspecified memory layout and an unspecified access pattern, which is a fair trade for private program state and a bad trade for anything that a device, a peer, or another thread also reads.
</details>
