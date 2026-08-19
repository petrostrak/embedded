# Week 4

## Concepts

- [x] **Set / clear / toggle / test a single bit.** `|= mask`, `&= ~mask`, `^= mask`, `& mask`.
  - [x] Note why the shift base should be `1UL` (or `1U`) and not `1`.
- [x] **Multi-bit fields.** Mask, shift, read-modify-write. Build the field value, clear the old field, OR the new one in.
  - [x] Mask *after* shifting so an oversized value can't spill into neighbouring fields.
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

This also avoids illegal intermediate states. Some hardware reacts to every write.

## Traps

### Shift by the full width of the type

`1u << 32` on a 32-bit `unsigned int` is **undefined behaviour**. A field that fills
the whole word breaks the standard mask formula.

```c
/* WRONG when width == 32 */
mask = ((1u << width) - 1u) << shift;

/* safe for width 1..32 */
mask = (0xFFFFFFFFu >> (32u - width)) << shift;
```

### Signed shift

`1 << 31` is undefined behaviour, because the result does not fit in a signed
`int`. Always use unsigned literals: `1u << 31`. Give all your mask constants a
`u` suffix.

### Integer promotion with small types

Anything smaller than `int` is promoted to `int` before the operation. The `~`
operator then works on 32 bits, not 8 or 16.

```c
uint16_t reg = 0xF0F0u;

if (~reg == 0x0F0Fu) { }        /* FALSE: ~reg is int 0xFFFF0F0F */
if ((uint16_t)~reg == 0x0F0Fu) { }   /* TRUE */
```

Assignment back into a `uint16_t` truncates, so `reg &= ~mask;` is usually correct
by accident. Comparisons and shifts are where it bites. Cast the result explicitly
and enable `-Wconversion`.

### Read-modify-write is not atomic

Three separate steps. An interrupt or a second core can change the register between
your read and your write. Your write then destroys their change.

Options:
- Disable interrupts around the sequence (short critical section).
- Use the hardware's dedicated set / clear registers, if it has them.
- Use `atomic_fetch_and` / `atomic_fetch_or` from `<stdatomic.h>` for normal memory.

### Write-1-to-clear registers

Read-modify-write is **wrong** for these. If a status bit reads as 1 and you write
the word back unchanged, you clear that bit. Write a fresh value with only the bits
you want to clear.

### Reserved bits

Read-modify-write preserves them, which is what most datasheets require. Do not
replace it with a blind full-word write unless the datasheet tells you the reset
value of every reserved bit.

### Signed fields

A field can hold a two's complement number. A plain shift-and-mask gives you an
unsigned value, so you must extend the sign yourself.

```c
static inline int32_t field_get_signed(uint32_t reg,
                                       uint32_t shift,
                                       uint32_t width)
{
    uint32_t raw  = (reg >> shift) & ((1u << width) - 1u);
    uint32_t sign = 1u << (width - 1u);
    return (int32_t)((raw ^ sign) - sign);
}
```

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
