# Week 4

## Concepts

- [x] **Set / clear / toggle / test a single bit.** `|= mask`, `&= ~mask`, `^= mask`, `& mask`.
  - [x] Note why the shift base should be `1UL` (or `1U`) and not `1`.
- [x] **Multi-bit fields.** Mask, shift, read-modify-write. Build the field value, clear the old field, OR the new one in.
  - [x] Mask *after* shifting so an oversized value can't spill into neighbouring fields.
- [x] **Why read-modify-write on a hardware register is a race.** Three bus accesses; an interrupt (or another master) landing between the read and the write loses its change.
  - [x] Atomic set/reset registers exist for exactly this. On the F3's GPIO: `BSRR` (set/reset, one write) and `BRR` (reset only).
  - [x] Note which registers you *must* still RMW (e.g. `MODER`), and how you protect those instead.
- [x] **Struct bitfields** — and why most embedded style guides ban them.
  - [x] Allocation order, straddling of storage units, and padding are implementation-defined.
  - [x] No guarantee about how many bus accesses a bitfield write becomes — fatal for hardware registers.
- [x] **`union` type punning vs `memcpy`.** `memcpy` is the portable way; union punning is legal in C (unlike C++) but still trips alignment and strict-aliasing assumptions when pointers get involved.
  - [x] Confirm at `-O2` that `memcpy` of 4 bytes compiles to a single load/store, i.e. it costs nothing.
- [x] **Endianness.** Little vs big; byte swapping for protocol work.
  - [x] Write your own `bswap16`/`bswap32` rather than relying on `htons` and friends.
  - [ ] Note why serialising byte-by-byte into a `uint8_t[]` sidesteps the whole question.
- [x] **`make`:**
  - [x] Targets, prerequisites, recipes; how make decides something is out of date.
  - [x] Pattern rules (`%.o: %.c`).
  - [x] Automatic variables: `$@`, `$<`, `$^`.
  - [x] Variables and `:=` vs `=`.
  - [x] Header dependency tracking with `-MMD -MP` and `-include $(DEPS)`.
  - [x] `.PHONY` for `clean` / `all`.

## Project — `bitops.h` + a hand-written Makefile

### `bitops.h` — the macros

- [x] Write the core macros:
  ```c
  #define BIT(n)              (1UL << (n))
  #define SET_BITS(reg, m)    ((reg) |=  (m))
  #define CLR_BITS(reg, m)    ((reg) &= ~(m))
  #define TGL_BITS(reg, m)    ((reg) ^=  (m))
  #define FIELD_SET(reg, m, sh, v) \
      ((reg) = ((reg) & ~(m)) | (((v) << (sh)) & (m)))
  ```
- [x] Note why every parameter is parenthesised, and construct a call that breaks if you drop one.
- [x] Note where these macros evaluate an argument more than once, and why that matters (`SET_BITS(*p++, m)`).
- [x] Add a `FIELD_GET(reg, m, sh)` counterpart.

### The bit functions
- [x] Build everything with the running flag set:
  ```
  -Wall -Wextra -Werror -Wconversion -std=c11
  ```
### The Makefile

- [ ] Variables: `CC`, `CFLAGS`, `SRCS`, `OBJS`, `DEPS`, `TARGET`.
- [ ] Default target `all`.
- [ ] A pattern rule compiling `%.c` → `%.o` using `$<` and `$@`.
- [ ] A link rule using `$^`.
- [ ] Add `-MMD -MP` to `CFLAGS` and `-include $(DEPS)` near the bottom.
- [ ] `.PHONY: all clean test`, and a `clean` that removes objects, deps, and the binary.
- [ ] No CMake this week — you need to have felt this before you let a tool do it.

### Prove the incremental build

- [ ] `make` from clean — everything compiles.
- [ ] `make` again — nothing compiles (`make: Nothing to be done`).
- [ ] `touch` one `.c` — exactly one object rebuilds, then relink.
- [ ] `touch bitops.h` — every object that includes it rebuilds, and nothing that doesn't.
- [ ] Delete the `.d` files and repeat the header test — watch it silently fail to rebuild. That's what `-MMD` is for.
- [ ] Record the before/after so you can explain it later.

## Done when

- [x] Touching a header rebuilds exactly the right objects and nothing else.
- [x] You can read your Makefile top to bottom and explain every line without guessing.
- [x] You can explain why RMW on a GPIO register is a race, and what `BSRR` does about it.

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
typedef struct
{
  uint32_t DMA1EN : 1;
  uint32_t DMA2EN : 1;
  uint32_t SRAMEN : 1;
  uint32_t Res0x3 : 1;
  uint32_t FLITFEN : 1;
  uint32_t FMCEN_1 : 1;
  uint32_t CRCEN : 1;
  uint32_t Res0x7_15 : 9;
  uint32_t IOPHEN_1 : 1;
  uint32_t IOPAEN : 1;
  uint32_t IOPBEN : 1;
  uint32_t IOPCEN : 1;
  uint32_t IOPDEN : 1;
  uint32_t IOPEEN : 1;
  uint32_t IOPFEN : 1;
  uint32_t IOPGEN_1 : 1;
  uint32_t TSCEN : 1;
  uint32_t Res0x25_27 : 3;
  uint32_t ADC12EN : 1;
  uint32_t ADC34EN : 1;
  uint32_t Res0x30_31 : 2;
} RCC_AHBENR_t;
```

The intent looks clear: pack all logical fields into 4 bytes, and read or write them by name.

```c
#define STATE_HIGH (1)
#define RCC_ADDR ((volatile RCC_AHBENR_t *)0x40021014UL)

// Enable the clock for GPIOE peripheral in the AHBENR
RCC_ADDR->IOPEEN = STATE_HIGH;
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

<details>
<summary>union vs memcpy</summary>

You have 32 bits. You want to see them as a `float` in one place and as a `uint32_t` in another. Typical cases:

- IEEE-754 bit manipulation (extract the exponent, build a NaN, fast inverse square root).
- Parsing a network packet or a file header out of a `unsigned char` buffer.
- Writing a value to a DMA descriptor or a serialisation buffer.
- Hashing a `double`.

C gives you three ways to do it. One is undefined behaviour, one is legal C but fragile, and one is portable.

## The three techniques

### 1. Pointer cast — undefined behaviour

```c
/* WRONG */
float bits_to_float_cast(uint32_t bits)
{
    return *(float *)&bits;
}
```

This breaks the **strict aliasing rule**. An object may be accessed only through an lvalue of a compatible type, of a signed/unsigned variant of it, or of a character type. A `uint32_t` object accessed through a `float` lvalue is none of those.

GCC warns about this exact form:

```
warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
```

It also breaks alignment. `&bits` has the alignment of `uint32_t`, which is not required to satisfy the alignment of `float`.

### 2. Union — legal in C, undefined in C++

```c
union u32f { uint32_t u; float f; };

float bits_to_float_union(uint32_t bits)
{
    union u32f x;
    x.u = bits;
    return x.f;          /* read a member that was not the last one written */
}
```

C11 6.5.2.3 paragraph 3 and footnote 95 permit this. If you read a member other than the one last written, the bytes of the object representation are reinterpreted in the new type. This is deliberate. It is not undefined behaviour in C.

**C++ does not allow it.** In C++ this reads a non-active union member, which is undefined behaviour. C++20 added `std::bit_cast` for the job. If a header is shared between C and C++, union punning is not portable across the two languages, even though every mainstream compiler accepts it in practice.

### 3. `memcpy` — always correct

```c
float bits_to_float_memcpy(uint32_t bits)
{
    float f;
    memcpy(&f, &bits, sizeof f);
    return f;
}
```

`memcpy` copies bytes. Access through a character type is explicitly exempt from the aliasing rule, and `memcpy` has no alignment requirement on either operand. This form is valid C, valid C++, valid on every alignment-strict CPU, and valid under every optimisation level.

## Why the union is still fragile

The union is legal, but it does not solve every problem the pointer cast has.

### It stops working when a pointer escapes

The permission applies to an access **through the union object**. When a bare pointer to a member leaves the union, the compiler no longer sees a union, and strict aliasing applies again.

```c
/* alias.c */
int alias_bug(uint32_t *u, float *f)
{
    *u = 1;
    *f = 2.0f;      /* the compiler assumes this cannot touch *u */
    return *u;
}
```

```console
$ gcc -O2 -S -masm=intel alias.c
alias_bug:
        mov     DWORD PTR [rdi], 1
        mov     eax, 1                    ; <-- return value folded to 1
        mov     DWORD PTR [rsi], 0x40000000
```

The function returns the constant 1, even if the caller passes the same address twice. With `-fno-strict-aliasing` the reload comes back:

```console
$ gcc -O2 -fno-strict-aliasing -S -masm=intel alias.c
alias_bug:
        mov     DWORD PTR [rdi], 1
        mov     DWORD PTR [rsi], 0x40000000
        mov     eax, DWORD PTR [rdi]      ; <-- reloaded
```

**GCC produced no warning at all for this version.** The single-expression cast in section 2.1 is warned about. The two-pointer form is silent. Do not rely on the warning to find these bugs.

### Other union hazards

- **Trap representations.** Not every bit pattern is a valid value of every type. Reading such a pattern is undefined. This matters for `_Bool`, for pointers, and for `long double` on x87 (10 bytes of value in 12 or 16 bytes of storage).
- **Padding.** `sizeof(union)` is the size of the largest member, rounded up for alignment. The trailing bytes are unspecified. If the members have different sizes, the extra bytes are garbage.
- **Endianness.** The union does not change byte order. A `union { uint32_t u; uint8_t b[4]; }` gives a different `b[0]` on a big-endian machine.
- **Size assumptions.** `sizeof(float) == sizeof(uint32_t)` is not guaranteed by the standard. Assert it.
- **Anonymous or partial writes.** Writing `x.b[0]` then reading `x.u` leaves the other three bytes indeterminate.

`memcpy` avoids the first, second and fifth of these. It does not fix endianness, and it does not fix a size mismatch — but `memcpy` with an explicit size makes the size mismatch a compile-time error rather than silent truncation.

## The cost — measured, not assumed

The usual objection to `memcpy` is "it is a function call". At `-O2` it is not. GCC treats `memcpy` with a **constant** size as a builtin and lowers it to plain loads and stores.

### The three techniques compile to the same instruction

```c
/* pun.c */
float bits_to_float_memcpy(uint32_t b) { float f; memcpy(&f, &b, sizeof f); return f; }
float bits_to_float_union (uint32_t b) { union u32f x; x.u = b; return x.f; }
float bits_to_float_cast  (uint32_t b) { return *(float *)&b; }
```

```console
$ gcc -O2 -S -masm=intel pun.c
bits_to_float_memcpy:
        movd    xmm0, edi
        ret
bits_to_float_union:
        movd    xmm0, edi
        ret
bits_to_float_cast:
        movd    xmm0, edi
        ret
```

Identical. One instruction. The `memcpy` version costs nothing, and it is the only one of the three that is correct in every context.

The 8-byte case is the same:

```console
bits_to_double_memcpy:
        movq    xmm0, rdi
        ret
```

### A 4-byte copy between memory operands

```c
void c4(void *d, const void *s) { memcpy(d, s, 4); }
```

```console
c4:
        mov     eax, DWORD PTR [rsi]      ; one 32-bit load
        mov     DWORD PTR [rdi], eax      ; one 32-bit store
        ret
```

**One load, one store.** No call, no loop, no length check.

### This holds at every optimisation level and in freestanding mode

| Flags | Result for `memcpy(d, s, 4)` |
| --- | --- |
| `-O2` | one load, one store |
| `-Os` | one load, one store |
| `-O2 -ffreestanding` | one load, one store |
| `-O2 -fno-builtin` | one load, one store |
| `-O0` | still inlined — a load and a store through the stack slots, **no call** |

Even `-O0` does not emit a call for a 4-byte constant-size copy:

```console
$ gcc -O0 -S -masm=intel sizes.c
c4:
        push    rbp
        mov     rbp, rsp
        mov     QWORD PTR -8[rbp], rdi
        mov     QWORD PTR -16[rbp], rsi
        mov     rax, QWORD PTR -16[rbp]
        mov     edx, DWORD PTR [rax]      ; load
        mov     rax, QWORD PTR -8[rbp]
        mov     DWORD PTR [rax], edx      ; store
        pop     rbp
        ret
```

`-ffreestanding` and `-fno-builtin` are the two flags most likely to appear in an embedded build. Neither one restores the call for a small constant size. GCC still expands `memcpy` because the C standard reserves the name; you only lose the expansion if you also pass `-fno-builtin-memcpy` or compile with a toolchain that has no builtin at all.

### Larger sizes stay inline; runtime sizes do not

```console
c8:                                  ; memcpy(d, s, 8)
        mov     rax, QWORD PTR [rsi]
        mov     QWORD PTR [rdi], rax

c12:                                 ; memcpy(d, s, 12)
        mov     rax, QWORD PTR [rsi]
        mov     QWORD PTR [rdi], rax
        mov     eax, DWORD PTR 8[rsi]
        mov     DWORD PTR 8[rdi], eax

c64:                                 ; memcpy(d, s, 64) -> four SSE pairs
        movdqu  xmm0, XMMWORD PTR [rsi]
        movups  XMMWORD PTR [rdi], xmm0
        ... (x4)

cn:                                  ; memcpy(d, s, n) with runtime n
        jmp     memcpy@PLT            ; <-- a real call
```

**The rule: a constant size is free, a runtime size is a call.** The threshold above which GCC gives up and calls the library is target-dependent and controlled by `-mmemcpy-strategy` / `--param` settings, but small fixed sizes are always inlined.

### A whole header parse is free

```c
struct hdr { uint32_t magic; uint16_t len; uint16_t flags; uint32_t crc; };

void parse(struct hdr *h, const unsigned char *p)
{
    memcpy(&h->magic, p + 0, 4);
    memcpy(&h->len,   p + 4, 2);
    memcpy(&h->flags, p + 6, 2);
    memcpy(&h->crc,   p + 8, 4);
}
```

```console
parse:
        mov     eax, DWORD PTR [rsi]
        mov     DWORD PTR [rdi], eax
        movzx   eax, WORD PTR 4[rsi]
        mov     WORD PTR 4[rdi], ax
        movzx   eax, WORD PTR 6[rsi]
        mov     WORD PTR 6[rdi], ax
        mov     eax, DWORD PTR 8[rsi]
        mov     DWORD PTR 8[rdi], eax
        ret
```

Four `memcpy` calls, zero function calls, eight instructions. This is the correct way to parse a buffer. It has no aliasing problem, no alignment problem, and no bitfield layout problem.

Add the byte order and it stays free:

```c
uint32_t read_be32(const unsigned char *p)
{
    uint32_t v;
    memcpy(&v, p, 4);
    return __builtin_bswap32(v);
}
```

```console
read_be32:
        mov     eax, DWORD PTR [rdi]
        bswap   eax
        ret
```

## Practical patterns

### A `bit_cast` helper

```c
#include <string.h>
#include <stdint.h>

#define BIT_CAST(To, from)                                   \
    __builtin_choose_expr(                                   \
        sizeof(To) == sizeof(from),                          \
        ({ To _t; memcpy(&_t, &(from), sizeof(_t)); _t; }),  \
        (void)0)
```

Portable version without GNU extensions:

```c
static inline uint32_t f32_bits(float f)
{
    uint32_t u;
    _Static_assert(sizeof u == sizeof f, "float is not 32 bits");
    memcpy(&u, &f, sizeof u);
    return u;
}

static inline float bits_f32(uint32_t u)
{
    float f;
    _Static_assert(sizeof u == sizeof f, "float is not 32 bits");
    memcpy(&f, &u, sizeof f);
    return f;
}
```

Use `sizeof` on the **destination** object, never a hard-coded number and never `sizeof` of a pointer.

### Buffer parsing

```c
static inline uint16_t rd_le16(const unsigned char *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static inline uint32_t rd_le32(const unsigned char *p)
{
    uint32_t v;
    memcpy(&v, p, sizeof v);      /* host order */
    return v;                     /* add a bswap if the wire is big-endian */
}
```

The shift-and-or version is endian-independent and also compiles to a single load plus a `bswap` on GCC and Clang at `-O2`. Either form is fine. A `struct` overlay with a pointer cast is not.

## Other `memcpy` details worth knowing

- **Overlap is undefined.** `memcpy` requires non-overlapping regions. Use `memmove` when the ranges can overlap. GCC does not diagnose this reliably.
- **Null pointers are undefined even with `n == 0`.** `memcpy(NULL, NULL, 0)` is UB by the letter of the standard. Guard the pointer, not the length.
- **`restrict` in the prototype is real.** The declaration is `void *memcpy(void *restrict, const void *restrict, size_t)`. Passing overlapping pointers is a contract violation the optimiser may exploit.
- **The destination may still be a trap representation.** `memcpy` gets the bytes across safely, but if those bytes are not a valid value of the destination type, reading the destination is still undefined. This applies to `_Bool`, to pointers, and to `long double`. It does not apply to `float`/`double`/integers on any mainstream target, where every bit pattern is a value (a NaN is a value).
- **Volatile is not covered.** `memcpy` cannot be used on `volatile` objects with any guarantee about access width or count. For hardware registers use a `volatile` object of the exact width instead.

## Summary

| | Pointer cast | Union | `memcpy` |
| --- | --- | --- | --- |
| Valid C | **No** — UB | Yes | Yes |
| Valid C++ | No | **No** — UB | Yes |
| Survives strict aliasing | No | Only while the union stays visible | Yes |
| Safe when misaligned | No | Yes (the union is aligned for all members) | Yes |
| Works on a raw `unsigned char` buffer | No | Only after a copy | Yes |
| Cost at `-O2` for 4 bytes | 1 instruction | 1 instruction | **1 instruction** |
| Cost at `-O0` for 4 bytes | inline | inline | inline, no call |
| Cost with a runtime size | n/a | n/a | a library call |
| Compiler warns when you get it wrong | Sometimes | No | n/a |

**One sentence:** `memcpy` with a constant size is the portable spelling of type punning and it compiles to exactly the same one or two instructions as the unsafe alternatives, so there is no performance argument for anything else — use the union only for genuinely C-only code where you never let a pointer to a member escape.

## Useful flags for an audit:

| Flag | Purpose |
| --- | --- |
| `-Wstrict-aliasing=2` | catch the obvious cast forms |
| `-Wcast-align` | flag casts that increase the required alignment |
| `-Wcast-align=strict` | flag them even on targets that tolerate unaligned access |
| `-fsanitize=alignment,undefined` | runtime detection (Clang's coverage is better than GCC's) |
| `-fno-strict-aliasing` | a workaround for legacy code, not a fix |
</details>

<details>
<summary>Endianness</summary>

A `uint32_t` holds one number. In memory it occupies four bytes. Endianness is the answer to one question: **which byte goes first?**

Take the value `0x01020304`.

```
Little-endian (x86, ARM in normal mode, RISC-V):
  address:  +0   +1   +2   +3
  byte:     04   03   02   01        <- least significant byte first

Big-endian (network order, SPARC, m68k, some MIPS/PowerPC):
  address:  +0   +1   +2   +3
  byte:     01   02   03   04        <- most significant byte first
```

Confirmed in the container:

```c
uint32_t x = 0x01020304u;
uint8_t m[4];
memcpy(m, &x, 4);
printf("%02X %02X %02X %02X\n", m[0], m[1], m[2], m[3]);
```

```
host layout of 0x01020304: 04 03 02 01
```

### Key points

- **Endianness only exists when you look at the bytes.** Arithmetic never sees it. `x >> 8` gives `0x00010203` on every machine, big or little. Shifts are defined on the *value*, not on the memory layout.
- **It applies to memory and to the wire.** Any time a multi-byte value crosses a boundary — a socket, a file, a CAN frame, a flash sector, a shared buffer between two chips — someone must agree on the order.
- **Bit order is a separate question.** Endianness is about bytes. The order of bits inside a byte is fixed by the ISA for arithmetic, and is a bitfield-layout question for structs (see the bitfield note).
- **"Network byte order" means big-endian.** That is a convention from the IP protocol suite, not a law. Many modern protocols are little-endian: USB, PCI, Bluetooth LE, most Modbus RTU registers are big-endian but the framing is not, RISC-V debug, Protocol Buffers varints, and almost every ARM-vendor peripheral.
- **Some formats are mixed.** A packet can have a big-endian length and a little-endian payload. Do not assume one order for a whole message.

### The classic bug

```c
uint8_t buf[4] = { 0xDE, 0xAD, 0xBE, 0xEF };   /* big-endian bytes off the wire */
uint32_t v;
memcpy(&v, buf, 4);
```

```
memcpy of wire bytes -> 0xEFBEADDE   (wrong on LE, right on BE)
```

The copy is correct C — it is the *interpretation* that is wrong. This code passes every test on a big-endian build machine and fails in the field.

## Detecting endianness

### Compile time — the right way

GCC and Clang define these:

```c
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    /* little */
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    /* big */
#else
    #error "unsupported byte order"
#endif
```

Verified in the container:

```
#define __ORDER_LITTLE_ENDIAN__ 1234
#define __ORDER_BIG_ENDIAN__    4321
#define __ORDER_PDP_ENDIAN__    3412
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
```

MSVC does not define these. For portability across all toolchains, test the target macros (`_M_IX86`, `__ARMEL__`, and so on) or force the choice from the build system.

### Runtime — avoid it

```c
/* works, but it is a runtime branch that should not exist */
static int is_little(void)
{
    const uint16_t probe = 1;
    return *(const uint8_t *)&probe == 1;
}
```

This is one of the few cases where a pointer cast is legal, because the target type is a character type. Even so, prefer the compile-time macro. A runtime test cannot be used in a `static` initialiser, cannot drive `#if`, and hides the answer from the optimiser.

### `_Static_assert` your assumption

If your code only supports one order, say so at compile time rather than producing wrong data on the other:

```c
_Static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
               "this driver assumes a little-endian host");
```

## Why not `htons` / `htonl`

`htons`, `htonl`, `ntohs`, `ntohl` come from the BSD sockets API. They work fine on a Linux server. They are the wrong tool for embedded, protocol, and library code, for eight separate reasons.

**1. They only do one direction.** Host to *big-endian*. There is no `htole32` in POSIX. Half of the protocols you will meet are little-endian, and the standard set gives you nothing for them.

**2. They only do 16 and 32 bits.** There is no portable `htonll`. Timestamps, 64-bit counters, and file offsets are all 64-bit.

**3. They need `<arpa/inet.h>` or `<winsock2.h>`.** That pulls the sockets API into a firmware image that has no sockets. On a freestanding target the header may not exist at all.

**4. The names lie about the operation.** `htonl` reads as "host to network long", but it operates on a `uint32_t`, not a `long`. On a 64-bit platform `long` is 8 bytes. The name misleads readers.

**5. They are a no-op on big-endian hosts.** That is correct behaviour, but it means the swap path is never exercised on a big-endian build. Bugs in the surrounding code hide.

**6. They may be macros, functions, or both.** Whether a call is inlined depends on the libc. Some embedded libcs implement them as real out-of-line calls. Some evaluate the argument twice.

**7. `htons` at a struct member is a trap.** `htons(x)` returns a value in *network* order stored in a *host* variable. The type system does not track this. A `uint16_t` that has already been swapped looks exactly like one that has not. Double-swapping is a common and silent bug.

**8. They do not solve alignment.** They convert a value. You still have to get the value in and out of the buffer, which is where the real problem lives.

### What the compiler does with them

For the record, on GCC/glibc they are efficient:

```console
$ gcc -O2 -S -masm=intel h.c
f:                       ; htons
        mov     eax, edi
        rol     ax, 8
        ret
g:                       ; htonl
        mov     eax, edi
        bswap   eax
        ret
```

Two instructions. So the objection is **not** performance. It is portability, coverage, and clarity.

## Writing your own swap functions

Write them once, in a header, with no dependencies.

```c
/* byteorder.h */
#ifndef BYTEORDER_H
#define BYTEORDER_H

#include <stdint.h>

static inline uint16_t bswap16(uint16_t v)
{
    return (uint16_t)((v >> 8) | (v << 8));
}

static inline uint32_t bswap32(uint32_t v)
{
    return ((v & 0x000000FFu) << 24)
         | ((v & 0x0000FF00u) <<  8)
         | ((v & 0x00FF0000u) >>  8)
         | ((v & 0xFF000000u) >> 24);
}

static inline uint64_t bswap64(uint64_t v)
{
    return ((uint64_t)bswap32((uint32_t)v) << 32)
         |  (uint64_t)bswap32((uint32_t)(v >> 32));
}

#endif
```

### Why this is safe

- It is pure arithmetic. No pointers, no casts, no aliasing, no alignment.
- It works identically on a big-endian and a little-endian host, because shifts operate on the value.
- It has no headers beyond `<stdint.h>`, so it builds freestanding.
- It is `static inline`, so it costs nothing and needs no `.c` file.
- The `uint16_t` cast on the return of `bswap16` is required: the operands promote to `int`, and without the cast you get an `int` with rubbish in the upper bits on some paths and a `-Wconversion` warning.

### What the compiler makes of it

```console
$ gcc -O2 -S -masm=intel bs.c
t16:
        mov     eax, edi
        rol     ax, 8            ; one instruction
        ret
t32:
        mov     eax, edi
        bswap   eax              ; one instruction
        ret
t64:
        mov     rax, rdi
        bswap   rax              ; one instruction
        ret
```

**GCC recognises the idiom and emits the native swap instruction.** Your portable C is exactly as fast as `htonl`, and it also covers 64 bits and both directions.

### The one caveat: it needs `-O2`

At `-O1` GCC does not run the idiom recogniser:

```console
$ gcc -O1 -S -masm=intel bs.c
t32:
        sal     eax, 24
        shr     edx, 24
        or      eax, edx
        sal     edx, 8
        and     edx, 16711680
        or      eax, edx
        shr     edi, 8
        and     edi, 65280
        or      eax, edi
        ret                      ; 8 extra instructions
```

`-O2` and `-Os` both recognise it. If your build is stuck at `-O1`, or you are on a compiler that does not do this, add a builtin fast path:

```c
static inline uint32_t bswap32(uint32_t v)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(v);
#elif defined(_MSC_VER)
    return _byteswap_ulong(v);
#else
    return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) <<  8)
         | ((v & 0x00FF0000u) >>  8) | ((v & 0xFF000000u) >> 24);
#endif
}
```

Keep the portable branch. It is the definition of correctness; the builtins are an optimisation.

### Naming: encode the direction

Do not write `swap_if_needed`. Name the conversion by the two orders involved:

```c
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define hton16(v) bswap16(v)
  #define hton32(v) bswap32(v)
  #define htole16(v) (v)
  #define htole32(v) (v)
#else
  #define hton16(v) (v)
  #define hton32(v) (v)
  #define htole16(v) bswap16(v)
  #define htole32(v) bswap32(v)
#endif
```

Better still: skip this layer entirely. Section 5 explains why.

## The better answer — serialise byte by byte

Every problem above comes from one decision: **storing a multi-byte value in memory and then arguing about its layout**. Do not store it. Build the bytes yourself.

```c
/* Big-endian (network order) */
static inline void put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8);
    p[3] = (uint8_t)(v      );
}

static inline uint32_t get_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24)
         | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8)
         | ((uint32_t)p[3]      );
}

/* Little-endian */
static inline void put_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v     );
    p[1] = (uint8_t)(v >> 8);
}

static inline uint16_t get_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
```

### Why this sidesteps the whole question

**1. There is no host byte order in the code.** `p[0] = v >> 24` says "the most significant byte goes at offset 0". Shifts are defined on the value. That statement is true on x86, on a big-endian PowerPC, and on a hypothetical middle-endian machine. Nothing needs to be conditional, so there is no `#if` and no build configuration to get wrong.

**2. There is no alignment requirement.** You are doing byte accesses. `p` can point anywhere. Confirmed at a deliberately misaligned offset:

```c
uint8_t b[8] = {0};
put_be32(b + 1, 0xDEADBEEFu);        /* offset 1 */
```
```
bytes: 00 DE AD BE EF
round-trip: DEADBEEF
```

**3. There is no aliasing problem.** Access through `uint8_t` is explicitly permitted against any object.

**4. The wire format is visible in the source.** A reviewer with the spec in hand can check `put_be32` against the diagram in one glance. There is no invisible dependency on a compiler macro.

**5. The direction cannot be double-applied.** `get_be32` takes bytes and returns a value. `put_be32` takes a value and writes bytes. The types make the direction obvious. Compare with `htons`, where a swapped and an unswapped `uint16_t` are the same type.

**6. It builds anywhere.** `<stdint.h>` only.

**7. It composes.** A parser is just a cursor walking a buffer, and the code reads like the protocol table.

### The cost — measured

This is the part people do not believe:

```console
$ gcc -O2 -S -masm=intel bs.c

put_be32:
        bswap   esi
        mov     DWORD PTR [rdi], esi     ; one swap, one store
        ret

get_be32:
        mov     eax, DWORD PTR [rdi]
        bswap   eax                      ; one load, one swap
        ret

put_le16:
        mov     WORD PTR [rdi], si       ; ONE store. No work at all.
        ret

get_le16:
        movzx   eax, WORD PTR [rdi]      ; ONE load. No work at all.
        ret
```

GCC recognises the shift-and-or pattern, merges the four byte accesses into a single word access, and adds a `bswap` only when the host order differs from the wire order. On a big-endian host the `bswap` disappears from `put_be32` and appears in `put_le16` instead — automatically, with no `#if` in your source.

**You write the most portable possible code and get the same instructions a hand-tuned cast would give you.** Same result at `-Os`. At `-O1` GCC keeps the four byte loads, which is still correct, just larger.

One nuance worth knowing: the merge into a single word access happens only when the compiler knows unaligned access is allowed on the target. On a Cortex-M0 build, or with `-mno-unaligned-access`, GCC keeps the byte loads — which is exactly what you want, because it is the only correct form there. You do not have to make that decision.

## A worked example — parsing a packet

The protocol:

| Offset | Size | Field | Order |
| --- | --- | --- | --- |
| 0 | 4 | magic | big |
| 4 | 2 | version | big |
| 6 | 2 | flags | little |
| 8 | 8 | timestamp | big |
| 16 | 4 | payload length | big |

Note the mixed order at offset 6. This is common in real formats and it is where a "just define a struct and swap the whole thing" approach falls apart.

```c
#include <stdint.h>
#include <stddef.h>

#define HDR_SIZE 20

struct hdr {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint64_t timestamp;
    uint32_t payload_len;
};

/* returns 0 on success */
int hdr_parse(struct hdr *out, const uint8_t *buf, size_t len)
{
    if (len < HDR_SIZE) {
        return -1;
    }

    out->magic       = get_be32(buf +  0);
    out->version     = get_be16(buf +  4);
    out->flags       = get_le16(buf +  6);   /* little, per the spec */
    out->timestamp   = get_be64(buf +  8);
    out->payload_len = get_be32(buf + 16);

    return 0;
}

size_t hdr_write(uint8_t *buf, size_t cap, const struct hdr *in)
{
    if (cap < HDR_SIZE) {
        return 0;
    }

    put_be32(buf +  0, in->magic);
    put_be16(buf +  4, in->version);
    put_le16(buf +  6, in->flags);
    put_be64(buf +  8, in->timestamp);
    put_be32(buf + 16, in->payload_len);

    return HDR_SIZE;
}
```

The struct here is **internal state only**. It is never overlaid on the buffer, never `memcpy`d to the wire, and its `sizeof` is irrelevant. `HDR_SIZE` is a constant taken from the spec, not from the compiler. That separation is the whole point.

### A cursor for longer messages

```c
struct rd { const uint8_t *p; const uint8_t *end; int err; };

static inline uint32_t rd_be32(struct rd *r)
{
    if (r->end - r->p < 4) { r->err = 1; return 0; }
    uint32_t v = get_be32(r->p);
    r->p += 4;
    return v;
}
```

Now a parser is a sequence of reads with one error check at the end. The bounds check is in one place, and no read can run past the buffer.

## Floats, and other things that need care

**Floating point.** IEEE-754 does not define a byte order; the host's integer endianness normally applies, but not always (ARM's old FPA format stored a `double` as two 32-bit words in the opposite order, which is why GCC still defines `__FLOAT_WORD_ORDER__` separately). Serialise a float by converting it to an integer first, then use the integer path:

```c
static inline void put_be_f32(uint8_t *p, float f)
{
    uint32_t u;
    _Static_assert(sizeof u == sizeof f, "float is not 32 bits");
    memcpy(&u, &f, sizeof u);      /* not a union, not a cast */
    put_be32(p, u);
}
```

`memcpy` here, for the reasons in the type-punning note.

**Signed integers.** Cast to the unsigned type of the same width, serialise that, and cast back on read. Two's complement is guaranteed from C23 and is universal in practice, but shifting a negative signed value is implementation-defined or undefined, so route it through unsigned:

```c
put_be32(p, (uint32_t)value);
int32_t value = (int32_t)get_be32(p);   /* implementation-defined pre-C23, fine everywhere real */
```

**Strings and byte arrays.** No endianness. Copy them as-is.

**Text formats.** JSON, XML, and ASCII protocols have no byte order problem at all. If the format allows it, this is the cheapest fix.

**Hardware registers.** A peripheral register has its own byte order, which may differ from the CPU's. Some SoCs have a byte-swap bit in a bus bridge. Read the datasheet; do not assume the register matches the core.

## Summary

| Question | Answer |
| --- | --- |
| Does endianness affect arithmetic? | No. Only memory layout and I/O. |
| Is network order big-endian? | Yes, but many protocols are not. Read the spec. |
| Should I use `htons`/`htonl`? | No. One direction, two widths, needs sockets headers. |
| Is a hand-written `bswap32` slow? | No. GCC emits `bswap` at `-O2` and `-Os`. |
| Do I need `#if __BYTE_ORDER__` in a parser? | No, if you serialise byte by byte. |
| Are byte-at-a-time reads slow? | No. GCC merges them into one load plus one `bswap`. |
| Is byte-at-a-time safe when misaligned? | Yes. That is one of the main reasons to use it. |
| How do I send a `float`? | `memcpy` it to a `uint32_t`, then use the integer path. |
| Can I overlay a struct on the buffer? | No. Byte order, alignment, and padding all break it. |

**One sentence:** keep every multi-byte value inside a register-width variable where endianness does not exist, and cross the memory boundary only through explicit `p[n] = v >> k` code — the compiler turns that back into a single load or store with a swap, so the portable version is also the fast version.

## Useful warnings for protocol code:

| Flag | Purpose |
| --- | --- |
| `-Wconversion` | catches the implicit narrowing in `p[0] = v >> 24` if the cast is missing |
| `-Wsign-conversion` | catches signed/unsigned mixups in shift expressions |
| `-Wcast-align=strict` | flags struct-overlay casts even on tolerant targets |
| `-Wpadded` | warns when a struct gains padding, useful while auditing |
| `-fsanitize=undefined` | catches out-of-range shifts at runtime |
</details>

<details>
<summary>make</summary>

`make` is a dependency engine with a build language bolted on. You describe
*what depends on what*, and `make` figures out the minimum work needed to bring
everything up to date.

It does **not** know C. It knows files, timestamps, and shell commands.

Run it with:

```sh
make            # build the first target in the file
make foo        # build target 'foo'
make -j8        # run up to 8 recipes in parallel
make -n         # dry run: print commands, execute nothing
```

By default `make` looks for `GNUmakefile`, then `makefile`, then `Makefile`.
Use `Makefile`.

> All examples here are GNU make. BSD make differs in several places.

## Targets, prerequisites, recipes

The basic unit is a **rule**:

```make
target: prerequisite1 prerequisite2
	recipe line 1
	recipe line 2
```

| Part | Meaning |
|---|---|
| **target** | The file that gets produced (or a name to act on). |
| **prerequisites** | Files that must exist and be up to date *before* the recipe runs. |
| **recipe** | Shell commands that produce the target from the prerequisites. |

A concrete rule:

```make
main.o: main.c
	gcc -c main.c -o main.o
```

### The tab rule

**Every recipe line must start with a real TAB character**, not spaces. This is
the single most common beginner error. The message you get is:

```
Makefile:5: *** missing separator.  Stop.
```

Configure your editor: for `Makefile`, disable "expand tabs to spaces".

### Each recipe line is its own shell

```make
bad:
	cd /tmp
	pwd        # prints your original directory, NOT /tmp
```

The `cd` happened in a shell that already exited. Chain with `&&` and `\`
instead:

```make
good:
	cd /tmp && pwd
```

### Recipe prefixes

```make
target:
	@echo "building"     # @  = do not echo the command itself
	-rm maybe-missing    # -  = ignore a non-zero exit status
```

Without `@`, make prints every command before running it. Without `-`, the
first failing command aborts the build.

## How make decides something is out of date

The algorithm, applied recursively:

1. To build target `T`, first bring all of `T`'s prerequisites up to date.
2. Then rebuild `T` if:
   - `T` does not exist, **or**
   - any prerequisite has a **modification time newer than `T`**.
3. Otherwise, do nothing and report `'T' is up to date.`

That is the whole rule. It is **timestamps only** — not content hashes, not
compiler flags, not file size.

### Worked example

```make
app: main.o util.o
	gcc main.o util.o -o app

main.o: main.c
	gcc -c main.c -o main.o

util.o: util.c
	gcc -c util.c -o util.o
```

You edit `util.c`:

```
util.c   is newer than  util.o   ->  rebuild util.o
util.o   is now newer than  app  ->  relink app
main.c   is older than  main.o   ->  skip main.o        <- the win
```

Only one compile plus one link, instead of two compiles plus one link.

### Consequences you will hit

- **Editing a header rebuilds nothing** unless you declared the dependency.
  This causes real, confusing bugs (stale struct layouts, ODR-style
  mismatches). Section 7 fixes it properly.
- **Changing `CFLAGS` rebuilds nothing.** No timestamp changed. Run
  `make clean` after changing flags, or use a flags-hash sentinel file.
- **`touch` alone triggers a rebuild**, because only the timestamp matters.
- **Clock skew matters.** Files from a network share or a restored archive can
  carry future timestamps and confuse make. It usually warns.

## Variables: `=` vs `:=`

Two flavours. The difference is *when the right-hand side is evaluated*.

### `:=` — simply expanded (evaluated once, immediately)

```make
CC := gcc
CFLAGS := -Wall -O2
SRCS := $(wildcard src/*.c)
```

The value is computed at the point of the assignment and frozen.

### `=` — recursively expanded (evaluated every time it is used)

```make
CC = gcc
CFLAGS = $(WARN) -O2
WARN = -Wall -Wextra          # defined AFTER, and it still works
```

`$(CFLAGS)` is not resolved until something uses it, so forward references are
fine.

### The trap

```make
# Recursive: infinite loop, make errors out
CFLAGS = $(CFLAGS) -g

# Simple: fine, appends to the current value
CFLAGS := $(CFLAGS) -g
```

Another trap — accidental repeated work:

```make
SRCS = $(shell find . -name '*.c')   # runs 'find' EVERY time $(SRCS) is used
SRCS := $(shell find . -name '*.c')  # runs 'find' exactly once
```

### Rule of thumb

> **Use `:=` by default.** Reach for `=` only when you deliberately want late
> evaluation (forward references, or a value that depends on the target being
> built).

### The other assignment operators

| Operator | Meaning |
|---|---|
| `:=` | Simply expanded. Evaluate now. |
| `=` | Recursively expanded. Evaluate on use. |
| `?=` | Assign only if the variable is not already set. |
| `+=` | Append. Keeps the flavour of the original assignment. |
| `::=` | POSIX spelling of `:=`. Identical behaviour. |
| `!=` | Run a shell command, assign its output (like `:= $(shell ...)`). |

`?=` is how you let users override things from the command line or the
environment:

```make
CC ?= gcc
PREFIX ?= /usr/local
```

```sh
make CC=clang            # command line wins over everything
```

### Trailing whitespace is significant

```make
DIR := build     # this comment leaves 'build' plus trailing spaces
```

Anything before the `#` is part of the value. Put comments on their own line.

## Automatic variables

Set by make inside each recipe, based on the rule being executed.

| Variable | Value | Typical use |
|---|---|---|
| `$@` | The target | Output filename |
| `$<` | The **first** prerequisite | Input to the compiler |
| `$^` | **All** prerequisites, duplicates removed | Inputs to the linker |
| `$+` | All prerequisites, duplicates kept, order preserved | Link order tricks |
| `$?` | Prerequisites **newer than the target** | Incremental archives |
| `$*` | The stem matched by `%` in a pattern rule | Deriving sibling names |
| `$(@D)` / `$(@F)` | Directory / file part of `$@` | `mkdir -p $(@D)` |
| `$(<D)` / `$(<F)` | Directory / file part of `$<` | |

### Applied

```make
app: main.o util.o log.o
	$(CC) $^ -o $@
#        |     |
#        |     +-- app
#        +-------- main.o util.o log.o

main.o: main.c config.h
	$(CC) $(CFLAGS) -c $< -o $@
#	                   |     |
#	                   |     +-- main.o
#	                   +-------- main.c   (NOT config.h)
```

This is the key idiom: **`$<` for compiling, `$^` for linking.**

Compiling with `$^` would be wrong — it would pass `config.h` to the compiler
as if it were a source file.

### Why `$*` is useful

In `%.o: %.c`, building `src/parser.o` gives `$* = src/parser`. So you can name
a sibling file:

```make
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -MF $*.d
```

### Creating output directories

`$(@D)` saves you from "No such file or directory" on a fresh clone:

```make
build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
```

## Pattern rules

Writing one rule per object file does not scale. A **pattern rule** uses `%` as
a wildcard stem:

```make
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
```

Read it as: "to make any `X.o`, you need `X.c`, and here is how".

The `%` matches the same text — the **stem** — on both sides. For
`parser.o`, the stem is `parser` and the prerequisite becomes `parser.c`.

### With separate source and build directories

```make
SRC_DIR := src
OBJ_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
```

`patsubst` transforms `src/main.c src/util.c` into
`build/main.o build/util.o`.

### Adding a shared prerequisite

Every object should rebuild when `config.h` changes:

```make
%.o: %.c config.h
	$(CC) $(CFLAGS) -c $< -o $@
```

This works but is coarse — *every* object rebuilds even if it never includes
`config.h`. Section 7 does it precisely instead.

### Static pattern rules

A pattern rule applies to anything that matches. A **static pattern rule**
restricts it to an explicit list:

```make
$(OBJS): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
```

Format: `targets: target-pattern: prereq-pattern`. Safer in larger projects,
because it cannot accidentally match unrelated files.

### Built-in rules

GNU make already ships a `%.o: %.c` rule that runs
`$(CC) $(CPPFLAGS) $(CFLAGS) -c`. This is why a bare `make main` sometimes
works with no Makefile at all. It also causes surprises. To see them:

```sh
make -p | less        # print the full database, built-ins included
```

Disable them with `make -r`, or in the file:

```make
MAKEFLAGS += --no-builtin-rules
```

## Header dependency tracking (`-MMD -MP`)

### The problem

```make
main.o: main.c
	$(CC) -c $< -o $@
```

You edit `util.h`, which `main.c` includes. Make sees `main.c` unchanged and
skips the rebuild. You now have an object file compiled against an old header.
The build "succeeds" and the program misbehaves.

Writing the dependencies by hand does not work — `#include` chains are deep and
they change constantly.

### The solution

The compiler already parses every `#include`. Ask it to write down what it
found.

| Flag | Effect |
|---|---|
| `-MD` | Generate a `.d` dependency file **as a side effect of compiling**. |
| `-MMD` | Same, but **skip system headers** (`<stdio.h>` etc.). Usually what you want. |
| `-MP` | Add a dummy, prerequisite-less target for each header. |
| `-MF <file>` | Choose where to write the `.d` file. |

Add them to `CPPFLAGS`:

```make
DEPFLAGS := -MMD -MP
```

Compiling `main.c` then produces `main.o` **and** `main.d` containing something
like:

```make
main.o: main.c util.h config.h
```

That is a valid make rule. Include it, and make now knows about the headers.

### Why `-MP`

Without `-MP`, deleting or renaming `util.h` breaks the build:

```
make: *** No rule to make target 'util.h', needed by 'main.o'.  Stop.
```

Make is reading a stale `.d` file that still references the deleted header, and
it has no way to produce it. `-MP` appends empty targets:

```make
main.o: main.c util.h config.h

util.h:
config.h:
```

Now make can "build" `util.h` by doing nothing, so it proceeds and recompiles
`main.c` — which regenerates a correct `.d` file. `-MP` costs nothing. Always
use it.

### Why `-include` and not `include`

```make
DEPS := $(OBJS:.o=.d)
-include $(DEPS)
```

- `$(OBJS:.o=.d)` is substitution reference shorthand: it turns
  `build/main.o` into `build/main.d`.
- **`include`** fails hard if a file is missing. On a clean checkout no `.d`
  files exist yet, so the build would never start.
- **`-include`** (leading dash) silently ignores missing files. First build:
  nothing to include, everything compiles anyway, and the `.d` files appear as
  a side effect. Second build onwards: full header tracking.

This is a genuine bootstrap: the information make needs is produced by the
build it is about to run. It works because a missing `.d` file only happens
when the corresponding `.o` is also missing, and that object is getting
compiled regardless.

### `-MMD` versus `-MD`

`-MMD` omits system headers. Upgrading libc will not force a full rebuild, and
your `.d` files stay small. If you need bit-exact reproducibility against
toolchain changes, use `-MD`. For everyday work, `-MMD`.

## `.PHONY`

A phony target is a *name for an action*, not a file to produce.

```make
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
```

### Why it is needed

Without `.PHONY`, if a file named `clean` ever exists in your directory:

```sh
$ touch clean
$ make clean
make: 'clean' is up to date.
```

Make applied its normal logic: the target `clean` exists, it has no
prerequisites, so nothing can be newer than it. Nothing to do.

`.PHONY` tells make "this name is never a file — always run the recipe, never
check timestamps". It also skips the implicit-rule search, which makes phony
targets slightly faster.

### The usual set

```make
.PHONY: all clean install test run debug release help
```

### `all` and the default target

The **first** target in the file is what a bare `make` builds. Convention is to
make it a phony `all` that depends on the real deliverables:

```make
.PHONY: all
all: $(TARGET)
```

Put this near the top. It documents intent, and it lets you add a second
deliverable later without changing how `make` behaves. Alternatively, be
explicit:

```make
.DEFAULT_GOAL := all
```

### A phony target with a real file name is a trap

```make
.PHONY: test
test: $(TARGET)          # and 'test' is also a directory in your repo
	./$(TARGET) --selftest
```

Marking a real file phony means it gets rebuilt every time, and anything
depending on it does too. Only mark true actions as phony.

## Complete reference Makefile

```make
# ---- Toolchain -------------------------------------------------------------
CC       := gcc
CFLAGS   := -std=c17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS :=
LDFLAGS  :=
LDLIBS   := -lm

# ---- Layout ----------------------------------------------------------------
SRC_DIR := src
OBJ_DIR := build
TARGET  := app

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

DEPFLAGS := -MMD -MP

# ---- Goals -----------------------------------------------------------------
.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: debug
debug: CFLAGS := -std=c17 -Wall -Wextra -g3 -O0 -fsanitize=address,undefined
debug: LDFLAGS += -fsanitize=address,undefined
debug: clean all

.PHONY: run
run: $(TARGET)
	./$(TARGET)

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: help
help:
	@echo "Targets: all debug run clean help"

-include $(DEPS)
```

Notes on the above:

- `debug: CFLAGS := ...` is a **target-specific variable**. The new value
  applies to that target and everything it depends on. Because flag changes do
  not alter timestamps, `debug` depends on `clean` to force a full rebuild.
- `CPPFLAGS` is the conventional home for `-I` and `-D`; `CFLAGS` for language
  and optimisation options. Keeping them separate matches the built-in rules
  and what users expect to override.
- `-include $(DEPS)` sits at the bottom by convention, so no included file can
  accidentally become the default goal.

## Quick reference

### Useful functions

```make
$(wildcard src/*.c)                       # glob the filesystem
$(patsubst %.c,%.o,$(SRCS))               # pattern substitution
$(SRCS:.c=.o)                             # shorthand for the above
$(notdir src/main.c)                      # -> main.c
$(basename src/main.c)                    # -> src/main
$(addprefix build/,main.o util.o)         # -> build/main.o build/util.o
$(shell git rev-parse --short HEAD)       # run a command
$(foreach d,$(DIRS),-I$(d))               # loop
$(if $(DEBUG),-g,-O2)                     # conditional
```

### Useful command-line flags

| Flag | Effect |
|---|---|
| `-n` | Dry run. Print commands without running them. |
| `-j N` | Run N recipes in parallel. |
| `-B` | Force rebuild everything. |
| `-k` | Keep going after errors. |
| `-C dir` | Change directory first. |
| `-p` | Dump the variable and rule database. |
| `-r` | Disable built-in rules. |
| `--debug=b` | Explain why each target is being rebuilt. |
| `-t` | Touch targets instead of rebuilding (dangerous). |

### Debugging a Makefile

```make
$(info CFLAGS is $(CFLAGS))          # print during parse
$(warning suspicious value: $(X))    # print with file:line prefix
$(error missing required variable)   # print and abort
```

```sh
make --debug=b            # why is this rebuilding?
make -p | grep '^CFLAGS'  # what is this variable's final value?
make -n                   # what would run?
```

### Common errors decoded

| Message | Cause |
|---|---|
| `missing separator` | Spaces instead of a TAB on a recipe line. |
| `No rule to make target 'x.h', needed by 'y.o'` | Stale `.d` file for a deleted header. Add `-MP`. |
| `Nothing to be done for 'all'` | Target exists and is newer than its prerequisites. |
| `'clean' is up to date` | A file named `clean` exists, and `clean` is not `.PHONY`. |
| `recipe commences before first target` | An indented line before any rule. |
| Recursion error on `X = $(X) ...` | Use `:=`, not `=`. |


## Mental model, condensed

1. A rule says: **this file** comes from **these files** via **these commands**.
2. Make rebuilds a target when it is missing, or when a prerequisite has a
   newer timestamp. Timestamps only — nothing else.
3. Pattern rules generalise one rule to a whole class of files.
4. `$@` is what you are making. `$<` is the main input. `$^` is all the inputs.
5. `:=` evaluates now; `=` evaluates later. Prefer `:=`.
6. `-MMD -MP` lets the compiler write your header dependencies for you;
   `-include $(DEPS)` feeds them back in.
7. `.PHONY` marks names that are actions, not files.

Everything else in make is elaboration on these seven points.
</details>
