# Week 4

## Concepts

- [ ] **Set / clear / toggle / test a single bit.** `|= mask`, `&= ~mask`, `^= mask`, `& mask`.
  - [ ] Note why the shift base should be `1UL` (or `1U`) and not `1`.
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
