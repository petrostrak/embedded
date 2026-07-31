# The Embedded Systems in C Roadmap

## Integer promotion and the usual arithmetic conversions.
### Terminology

| Term | Plain meaning |
|---|---|
| **operand** | A value on one side of an operator. In `a + b`, both `a` and `b` are operands. |
| **rank** | A fixed order of the integer types. From highest to lowest: `long long`, `long`, `int`, `short`, `char`. Two types can be the same width but differ in rank. `int32_t` and `long` are both 32 bits wide, but `long` ranks higher. |
| **signed / unsigned** | A signed type can hold negative values. An unsigned type cannot. |
| **promote** | Convert a value to `int`, or to `unsigned int` if `int` cannot hold every value of the original type. |

### The seven steps

| Rule | What to do | Example |
|---|---|---|
| **1** | If either operand has a floating type, convert both operands to the wider floating type. Then stop. | `i32 / 2.0f` → both become `float`. Result: `float`. |
| **2** | Promote both operands. Do this to each operand on its own, before you compare them. | `u8 + u8` → each `uint8_t` becomes `int`. |
| **3** | If both operands now have the same type, stop. You are done. | `i32 + i32` → both are `int` already. Result: `int`. |
| **4** | If both operands are signed, or both are unsigned, convert the lower-ranked operand to the higher-ranked type. | `i32 + i64` → the `int` becomes `long long`. Result: `long long`. |
| **5** | If one operand is signed and one is unsigned, and the unsigned type ranks the same or higher, convert the signed operand to the unsigned type. | `u32 + i32` → equal rank, so the `int` becomes `unsigned int`. Result: `unsigned int`. Also `sz - i32`. |
| **6** | If the signed type ranks higher, and it can hold every value of the unsigned type, convert the unsigned operand to the signed type. | `u32 + i64` → `int64_t` holds every `uint32_t` value. Result: `long long`. |
| **7** | If the signed type ranks higher but cannot hold every value of the unsigned type, convert both operands to the unsigned version of the signed type. | `u32 + l` → `long` outranks `unsigned int`, but 32 bits cannot hold every `uint32_t` value. Result: `unsigned long`. |

