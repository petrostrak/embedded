# Cortex-M4 Instruction Set Reference (ARMv7E-M / Thumb-2)

The Cortex-M4 runs Thumb-2 only. It mixes 16-bit and 32-bit encodings. There is no
ARM (A32) state.

Reference documents:

- *ARMv7-M Architecture Reference Manual* — ARM DDI 0403
- *Cortex-M4 Technical Reference Manual* — ARM DDI 0439
- *Cortex-M4 Devices Generic User Guide* — ARM DUI 0553

---

## Contents

1. [Notation](#1-notation)
2. [Worked example: read-modify-write](#2-worked-example-read-modify-write)
3. [Move and register transfer](#3-move-and-register-transfer)
4. [Load and store](#4-load-and-store)
5. [Addressing modes](#5-addressing-modes)
6. [Arithmetic](#6-arithmetic)
7. [Logical](#7-logical)
8. [Shift and rotate](#8-shift-and-rotate)
9. [Compare and condition codes](#9-compare-and-condition-codes)
10. [Bit field and byte operations](#10-bit-field-and-byte-operations)
11. [Branch and control flow](#11-branch-and-control-flow)
12. [Saturating arithmetic (DSP)](#12-saturating-arithmetic-dsp)
13. [SIMD byte and halfword arithmetic (DSP)](#13-simd-byte-and-halfword-arithmetic-dsp)
14. [DSP multiply (DSP)](#14-dsp-multiply-dsp)
15. [Pack and extend (DSP)](#15-pack-and-extend-dsp)
16. [Floating point (Cortex-M4F only)](#16-floating-point-cortex-m4f-only)
17. [System, hints, and barriers](#17-system-hints-and-barriers)
18. [Register model](#18-register-model)

---

## 1. Notation

| Symbol | Meaning |
| --- | --- |
| `Rd` | Destination register |
| `Rn`, `Rm` | Source registers |
| `Ra` | Accumulator register |
| `Rt` | Transfer (stored) register |
| `Op2` | Flexible second operand: `#imm`, `Rm`, or `Rm, LSL #n` |
| `{S}` | Optional suffix. It updates the flags N, Z, C, V in the APSR. |
| `{cond}` | Optional condition. Outside a branch, it needs an `IT` block. |
| `[Rn, #off]` | Address = `Rn` + offset |
| `←` | "gets the value of" |

---

## 2. Worked example: read-modify-write

This C statement sets bit 5 of a peripheral register:

```c
GPIOA->ODR |= (1U << 5);        /* set PA5 high */
```

The compiler produces three instructions. `r0` holds the address of `ODR`
(`0x40020014` on an STM32F4):

```asm
LDR  r1, [r0]        ; 1. READ   ODR from the bus
ORR  r1, r1, #0x20   ; 2. MODIFY in a CPU register
STR  r1, [r0]        ; 3. WRITE  ODR back to the bus
```

| Instruction | Action |
| --- | --- |
| `LDR r1, [r0]` | Read the 32-bit word at the address in `r0` into `r1`. This is a real bus transaction. |
| `ORR r1, r1, #0x20` | `r1 = r1 \| 0x20`. `0x20` is bit 5. This is register work only. There is no bus access. |
| `STR r1, [r0]` | Write `r1` back to the address in `r0`. This is a second bus transaction. |

The compiler must produce all three instructions because `ODR` is `volatile`. It
cannot keep a cached copy, and it cannot merge the accesses.

**Warning:** the sequence is not atomic. If an interrupt handler writes a different
ODR bit between the `LDR` and the `STR`, the `STR` writes back the old value and
erases the change from the handler.

Three safe alternatives on STM32:

| Method | Code | Note |
| --- | --- | --- |
| BSRR register | `GPIOA->BSRR = (1U << 5);` | One `STR`, one bus write, atomic. Use this for GPIO. |
| Bit-banding | Write through the `0x42000000` alias region | The hardware does the read-modify-write. |
| Exclusive access | `LDREX` / `STREX` pair | The general method for shared variables in RAM. |

The safe version compiles to:

```asm
MOV  r1, #0x20       ; put bit 5 in a register
STR  r1, [r0]        ; single write to BSRR — atomic
```

`BSRR` is a set/reset register. A `1` in bits 0–15 sets the matching output. A `1`
in bits 16–31 clears it. A `0` does nothing. You never read the old value, so no
interrupt can corrupt the operation.

---

## 3. Move and register transfer

| Instruction | Name | Action |
| --- | --- | --- |
| `MOV Rd, Op2` | Move | `Rd ← Op2`. Copy a value into a register. |
| `MOVW Rd, #imm16` | Move Wide | `Rd ← imm16`. It writes the low halfword and clears the top halfword. |
| `MOVT Rd, #imm16` | Move Top | `Rd[31:16] ← imm16`. It keeps the low halfword. Use `MOVW` + `MOVT` to build a full 32-bit constant. |
| `MVN Rd, Op2` | Move NOT | `Rd ← NOT Op2`. Bitwise inverse. |
| `ADR Rd, label` | Address to Register | `Rd ← address of label`. The core calculates it from the PC. |
| `MRS Rd, spec_reg` | Move from Special Register | Read a system register, for example `PRIMASK`, `IPSR`, `CONTROL`, or `APSR`. |
| `MSR spec_reg, Rn` | Move to Special Register | Write a system register. |

---

## 4. Load and store

A load is a bus read. A store is a bus write.

| Instruction | Name | Action |
| --- | --- | --- |
| `LDR Rd, [Rn]` | Load Register | `Rd ← memory[Rn]`. 32-bit word. |
| `LDRB Rd, [Rn]` | Load Byte | Read 8 bits. It fills the top 24 bits with zeros. |
| `LDRH Rd, [Rn]` | Load Halfword | Read 16 bits. It fills the top 16 bits with zeros. |
| `LDRSB Rd, [Rn]` | Load Signed Byte | Read 8 bits. It copies bit 7 into the top 24 bits. |
| `LDRSH Rd, [Rn]` | Load Signed Halfword | Read 16 bits. It copies bit 15 into the top 16 bits. |
| `LDRD Rd, Rd2, [Rn]` | Load Doubleword | Read 64 bits into two registers. |
| `STR Rt, [Rn]` | Store Register | `memory[Rn] ← Rt`. 32-bit word. |
| `STRB Rt, [Rn]` | Store Byte | Write the low 8 bits only. |
| `STRH Rt, [Rn]` | Store Halfword | Write the low 16 bits only. |
| `STRD Rt, Rt2, [Rn]` | Store Doubleword | Write 64 bits from two registers. |
| `LDM Rn{!}, {reglist}` | Load Multiple | Read a block of words into several registers. `!` writes the new address back to `Rn`. |
| `STM Rn{!}, {reglist}` | Store Multiple | Write several registers to a block of memory. |
| `PUSH {reglist}` | Push | Store registers to the stack. It decrements SP first. |
| `POP {reglist}` | Pop | Load registers from the stack. It increments SP after. |
| `LDREX Rd, [Rn]` | Load Exclusive | Read a word and mark the address in the exclusive monitor. |
| `STREX Rd, Rt, [Rn]` | Store Exclusive | Write the word only if the mark is still valid. `Rd ← 0` for success, `1` for failure. |
| `LDREXB` / `LDREXH` | Load Exclusive Byte / Halfword | Same as `LDREX`, on 8 or 16 bits. |
| `STREXB` / `STREXH` | Store Exclusive Byte / Halfword | Same as `STREX`, on 8 or 16 bits. |
| `CLREX` | Clear Exclusive | Remove the mark. |
| `LDRT`, `LDRBT`, `LDRHT`, `LDRSBT`, `LDRSHT` | Load Unprivileged | Same as the load above, but the MPU checks the access with user rights. |
| `STRT`, `STRBT`, `STRHT` | Store Unprivileged | Same as the store above, with user rights. |

---

## 5. Addressing modes

These modes apply to all the loads and stores above.

| Form | Meaning |
| --- | --- |
| `[Rn]` | Address = `Rn` |
| `[Rn, #off]` | Offset. Address = `Rn + off`. `Rn` does not change. |
| `[Rn, #off]!` | Pre-index. Address = `Rn + off`, then `Rn ← Rn + off`. |
| `[Rn], #off` | Post-index. Address = `Rn`, then `Rn ← Rn + off`. |
| `[Rn, Rm]` | Register offset. Address = `Rn + Rm`. |
| `[Rn, Rm, LSL #n]` | Scaled register offset. Address = `Rn + (Rm << n)`. |
| `[PC, #off]` | PC-relative. The assembler uses it for literal pools. |

---

## 6. Arithmetic

| Instruction | Name | Action |
| --- | --- | --- |
| `ADD{S} Rd, Rn, Op2` | Add | `Rd ← Rn + Op2` |
| `ADC{S} Rd, Rn, Op2` | Add with Carry | `Rd ← Rn + Op2 + C`. Use it for multi-word addition. |
| `ADDW Rd, Rn, #imm12` | Add Wide | Add a 12-bit immediate. It does not change the flags. |
| `SUB{S} Rd, Rn, Op2` | Subtract | `Rd ← Rn − Op2` |
| `SBC{S} Rd, Rn, Op2` | Subtract with Carry | `Rd ← Rn − Op2 − NOT C` |
| `SUBW Rd, Rn, #imm12` | Subtract Wide | Subtract a 12-bit immediate. |
| `RSB{S} Rd, Rn, Op2` | Reverse Subtract | `Rd ← Op2 − Rn`. Use `RSB Rd, Rn, #0` to negate. |
| `MUL{S} Rd, Rn, Rm` | Multiply | `Rd ← (Rn × Rm)[31:0]`. The top 32 bits are lost. |
| `MLA Rd, Rn, Rm, Ra` | Multiply Accumulate | `Rd ← Ra + (Rn × Rm)` |
| `MLS Rd, Rn, Rm, Ra` | Multiply Subtract | `Rd ← Ra − (Rn × Rm)` |
| `SDIV Rd, Rn, Rm` | Signed Divide | `Rd ← Rn ÷ Rm`. It truncates toward zero. |
| `UDIV Rd, Rn, Rm` | Unsigned Divide | The same, unsigned. |
| `SMULL RdLo, RdHi, Rn, Rm` | Signed Multiply Long | 32 × 32 → 64-bit signed result. |
| `UMULL RdLo, RdHi, Rn, Rm` | Unsigned Multiply Long | 32 × 32 → 64-bit unsigned result. |
| `SMLAL RdLo, RdHi, Rn, Rm` | Signed Multiply Accumulate Long | Add the 64-bit product to the value in `RdHi:RdLo`. |
| `UMLAL RdLo, RdHi, Rn, Rm` | Unsigned Multiply Accumulate Long | The same, unsigned. |
| `UMAAL RdLo, RdHi, Rn, Rm` | Unsigned Multiply Accumulate Accumulate Long | `RdHi:RdLo ← (Rn × Rm) + RdHi + RdLo`. There is no carry out. |

---

## 7. Logical

| Instruction | Name | Action |
| --- | --- | --- |
| `AND{S} Rd, Rn, Op2` | Bitwise AND | `Rd ← Rn AND Op2` |
| `ORR{S} Rd, Rn, Op2` | Bitwise OR | `Rd ← Rn OR Op2`. Use it to set bits. |
| `ORN{S} Rd, Rn, Op2` | Bitwise OR NOT | `Rd ← Rn OR (NOT Op2)` |
| `EOR{S} Rd, Rn, Op2` | Exclusive OR | `Rd ← Rn XOR Op2`. Use it to toggle bits. |
| `BIC{S} Rd, Rn, Op2` | Bit Clear | `Rd ← Rn AND (NOT Op2)`. Use it to clear bits. |
| `TST Rn, Op2` | Test | Do `Rn AND Op2` and set the flags. It discards the result. |
| `TEQ Rn, Op2` | Test Equivalence | Do `Rn XOR Op2` and set the flags. It discards the result. |

---

## 8. Shift and rotate

| Instruction | Name | Action |
| --- | --- | --- |
| `LSL{S} Rd, Rn, #n` | Logical Shift Left | Move bits up. It puts zeros in at the bottom. Same as `× 2ⁿ`. |
| `LSR{S} Rd, Rn, #n` | Logical Shift Right | Move bits down. It puts zeros in at the top. |
| `ASR{S} Rd, Rn, #n` | Arithmetic Shift Right | Move bits down. It copies the sign bit in at the top. Same as signed `÷ 2ⁿ`. |
| `ROR{S} Rd, Rn, #n` | Rotate Right | Bits that leave the bottom re-enter at the top. |
| `RRX{S} Rd, Rn` | Rotate Right with Extend | Rotate right by 1 bit through the carry flag. |

---

## 9. Compare and condition codes

| Instruction | Name | Action |
| --- | --- | --- |
| `CMP Rn, Op2` | Compare | Do `Rn − Op2` and set the flags. It discards the result. |
| `CMN Rn, Op2` | Compare Negative | Do `Rn + Op2` and set the flags. |

Condition codes for `{cond}` and for `B{cond}`:

| Code | True when | Code | True when |
| --- | --- | --- | --- |
| `EQ` | Equal (Z=1) | `HI` | Unsigned higher |
| `NE` | Not equal | `LS` | Unsigned lower or same |
| `CS` / `HS` | Carry set, unsigned ≥ | `GE` | Signed ≥ |
| `CC` / `LO` | Carry clear, unsigned < | `LT` | Signed < |
| `MI` | Negative (N=1) | `GT` | Signed > |
| `PL` | Positive or zero | `LE` | Signed ≤ |
| `VS` | Overflow set | `AL` | Always |
| `VC` | Overflow clear | | |

---

## 10. Bit field and byte operations

| Instruction | Name | Action |
| --- | --- | --- |
| `CLZ Rd, Rm` | Count Leading Zeros | Count the zero bits above the highest set bit. |
| `RBIT Rd, Rm` | Reverse Bits | Reverse the order of all 32 bits. |
| `REV Rd, Rm` | Reverse Bytes in Word | Swap the endianness of a 32-bit value. |
| `REV16 Rd, Rm` | Reverse Bytes in Halfwords | Swap the endianness of each 16-bit half. |
| `REVSH Rd, Rm` | Reverse Signed Halfword | Swap the bytes of the low halfword, then sign extend. |
| `BFI Rd, Rn, #lsb, #width` | Bit Field Insert | Copy `width` bits from the bottom of `Rn` into `Rd` at position `lsb`. |
| `BFC Rd, #lsb, #width` | Bit Field Clear | Set a field of bits to zero. |
| `UBFX Rd, Rn, #lsb, #width` | Unsigned Bit Field Extract | Pull out a field and zero extend it. |
| `SBFX Rd, Rn, #lsb, #width` | Signed Bit Field Extract | Pull out a field and sign extend it. |
| `SXTB Rd, Rm` | Sign Extend Byte | Copy bit 7 into all higher bits. |
| `SXTH Rd, Rm` | Sign Extend Halfword | Copy bit 15 into all higher bits. |
| `UXTB Rd, Rm` | Zero Extend Byte | Clear all bits above bit 7. |
| `UXTH Rd, Rm` | Zero Extend Halfword | Clear all bits above bit 15. |

---

## 11. Branch and control flow

| Instruction | Name | Action |
| --- | --- | --- |
| `B{cond} label` | Branch | Jump to a label. |
| `BL label` | Branch with Link | Jump and save the return address in `LR`. This is a function call. |
| `BX Rm` | Branch Indirect | Jump to the address in `Rm`. `BX LR` returns from a function. |
| `BLX Rm` | Branch with Link Indirect | Call the address in `Rm`. This is a function pointer call. |
| `CBZ Rn, label` | Compare and Branch if Zero | Jump if `Rn == 0`. It does not change the flags. |
| `CBNZ Rn, label` | Compare and Branch if Not Zero | Jump if `Rn != 0`. |
| `TBB [Rn, Rm]` | Table Branch Byte | Jump with a byte-size offset table. Compilers use it for `switch`. |
| `TBH [Rn, Rm, LSL #1]` | Table Branch Halfword | The same, with halfword offsets. |
| `IT{x{y{z}}} cond` | If-Then | Make the next 1 to 4 instructions conditional. It replaces short branches. |

---

## 12. Saturating arithmetic (DSP)

Saturation clamps the result at the limit. It does not wrap around.

| Instruction | Name | Action |
| --- | --- | --- |
| `SSAT Rd, #n, Rm` | Signed Saturate | Clamp `Rm` to a signed `n`-bit range. |
| `USAT Rd, #n, Rm` | Unsigned Saturate | Clamp `Rm` to an unsigned `n`-bit range. |
| `SSAT16` / `USAT16` | Saturate Two Halfwords | The same, on both 16-bit lanes. |
| `QADD Rd, Rn, Rm` | Saturating Add | 32-bit add with saturation. |
| `QSUB Rd, Rn, Rm` | Saturating Subtract | 32-bit subtract with saturation. |
| `QDADD Rd, Rn, Rm` | Saturating Double and Add | It doubles `Rn` with saturation, then adds. |
| `QDSUB Rd, Rn, Rm` | Saturating Double and Subtract | It doubles `Rn` with saturation, then subtracts. |

---

## 13. SIMD byte and halfword arithmetic (DSP)

These instructions split the 32-bit register into lanes and work on all lanes at the
same time. The mnemonics follow one naming pattern:

| Part | Position | Meaning |
| --- | --- | --- |
| `S` / `U` | Prefix | Signed / unsigned lanes |
| `Q` | After the prefix | Saturating. The result clamps. |
| `H` | After the prefix | Halving. The core divides each result by 2 to prevent overflow. |
| `ADD` / `SUB` | Body | Add or subtract the matching lanes. |
| `ASX` / `SAX` | Body | Exchange the halfwords of `Rm` first. `ASX` = add top, subtract bottom. `SAX` is the opposite. |
| `8` / `16` | Suffix | Four 8-bit lanes, or two 16-bit lanes. |

| Family | Action |
| --- | --- |
| `SADD8`, `SADD16`, `UADD8`, `UADD16` | Add the lanes. The result wraps on overflow. |
| `SSUB8`, `SSUB16`, `USUB8`, `USUB16` | Subtract the lanes. The result wraps. |
| `QADD8`, `QADD16`, `QSUB8`, `QSUB16`, `UQADD8`, `UQADD16`, `UQSUB8`, `UQSUB16` | Add or subtract the lanes with saturation. |
| `SHADD8`, `SHADD16`, `SHSUB8`, `SHSUB16`, `UHADD8`, `UHADD16`, `UHSUB8`, `UHSUB16` | Add or subtract the lanes, then halve each result. |
| `SASX`, `SSAX`, `UASX`, `USAX`, `QASX`, `QSAX`, `SHASX`, `SHSAX`, `UHASX`, `UHSAX`, `UQASX`, `UQSAX` | Exchange the halfwords of `Rm`, then add one lane and subtract the other. These help with complex numbers. |
| `USAD8 Rd, Rn, Rm` | Sum of the absolute differences of four byte pairs. Video encoders use it. |
| `USADA8 Rd, Rn, Rm, Ra` | The same, plus an accumulator. |
| `SEL Rd, Rn, Rm` | Select each byte from `Rn` or `Rm`. The GE flags from a previous SIMD operation control the choice. |

---

## 14. DSP multiply (DSP)

`B` means the bottom halfword. `T` means the top halfword. `R` means the core
rounds the result. The `X` suffix exchanges the halfwords of the second operand
before the multiply.

| Instruction | Name | Action |
| --- | --- | --- |
| `SMULBB`, `SMULBT`, `SMULTB`, `SMULTT` | Signed Multiply Halfwords | Multiply two selected 16-bit halves. The result is 32-bit. |
| `SMLABB`, `SMLABT`, `SMLATB`, `SMLATT` | Signed Multiply Accumulate Halfwords | The same, plus an accumulator. |
| `SMULWB`, `SMULWT` | Signed Multiply Word by Halfword | 32 × 16. It keeps the top 32 bits of the 48-bit result. |
| `SMLAWB`, `SMLAWT` | Signed Multiply Accumulate Word by Halfword | The same, plus an accumulator. |
| `SMLALBB`, `SMLALBT`, `SMLALTB`, `SMLALTT` | Signed Multiply Accumulate Long Halfwords | 16 × 16 added to a 64-bit accumulator. |
| `SMUAD{X}` | Signed Dual Multiply Add | Multiply both halfword pairs and add the two products. This is a 2-tap dot product. |
| `SMUSD{X}` | Signed Dual Multiply Subtract | Multiply both pairs and subtract one product from the other. |
| `SMLAD{X}`, `SMLSD{X}` | Signed Dual Multiply Accumulate / Subtract | The same, plus a 32-bit accumulator. FIR filters use these. |
| `SMLALD{X}`, `SMLSLD{X}` | Signed Dual Multiply Accumulate Long | The same, with a 64-bit accumulator. |
| `SMMUL{R}` | Signed Most Significant Word Multiply | 32 × 32. It keeps the top 32 bits. |
| `SMMLA{R}`, `SMMLS{R}` | Signed MSW Multiply Accumulate / Subtract | The same, plus an accumulator. |

---

## 15. Pack and extend (DSP)

| Instruction | Name | Action |
| --- | --- | --- |
| `PKHBT Rd, Rn, Rm` | Pack Halfword Bottom Top | `Rd` gets the low half of `Rn` and the high half of `Rm`. |
| `PKHTB Rd, Rn, Rm` | Pack Halfword Top Bottom | `Rd` gets the high half of `Rn` and the low half of `Rm`. |
| `SXTAB`, `SXTAH`, `UXTAB`, `UXTAH` | Extend and Add | Extend a byte or halfword of `Rm`, then add it to `Rn`. |
| `SXTB16`, `UXTB16` | Extend Two Bytes | Extend two bytes into two halfword lanes. |
| `SXTAB16`, `UXTAB16` | Extend Two Bytes and Add | The same, then add to `Rn` per lane. |

---

## 16. Floating point (Cortex-M4F only)

The FPU is FPv4-SP. It has its own registers: `S0`–`S31`, or `D0`–`D15` as pairs.
All arithmetic is 32-bit `float`. There is **no** double-precision arithmetic in
hardware.

| Instruction | Name | Action |
| --- | --- | --- |
| `VADD.F32 Sd, Sn, Sm` | Add | `Sd ← Sn + Sm` |
| `VSUB.F32 Sd, Sn, Sm` | Subtract | `Sd ← Sn − Sm` |
| `VMUL.F32 Sd, Sn, Sm` | Multiply | `Sd ← Sn × Sm` |
| `VNMUL.F32 Sd, Sn, Sm` | Negated Multiply | `Sd ← −(Sn × Sm)` |
| `VDIV.F32 Sd, Sn, Sm` | Divide | `Sd ← Sn ÷ Sm`. It takes 14 cycles. |
| `VSQRT.F32 Sd, Sm` | Square Root | `Sd ← √Sm`. It takes 14 cycles. |
| `VMLA.F32` / `VMLS.F32` | Multiply Accumulate / Subtract | `Sd ← Sd ± (Sn × Sm)`. It rounds twice. |
| `VNMLA.F32` / `VNMLS.F32` | Negated Multiply Accumulate / Subtract | The same with a negated result. |
| `VFMA.F32` / `VFMS.F32` | Fused Multiply Accumulate / Subtract | The same, but it rounds only once. The result is more exact. |
| `VFNMA.F32` / `VFNMS.F32` | Fused Negated forms | Negated fused variants. |
| `VABS.F32 Sd, Sm` | Absolute Value | It clears the sign bit. |
| `VNEG.F32 Sd, Sm` | Negate | It inverts the sign bit. |
| `VCMP{E}.F32 Sd, Sm` | Compare | Compare and set the FPSCR flags. The `E` form also signals a quiet NaN. |
| `VCVT` / `VCVTR` | Convert | Convert between `float` and signed or unsigned integer. `R` uses the FPSCR rounding mode. |
| `VCVTB` / `VCVTT` | Convert Half Precision | Convert to or from 16-bit `half`, in the bottom or top halfword. |
| `VMOV` | Move | Move between FPU registers, between an FPU register and a core register, or load an immediate. |
| `VLDR Sd, [Rn]` | Load FPU Register | Read one FPU register from memory. |
| `VSTR Sd, [Rn]` | Store FPU Register | Write one FPU register to memory. |
| `VLDM` / `VSTM` | Load / Store Multiple | Read or write a block of FPU registers. |
| `VPUSH` / `VPOP` | Push / Pop | Save or restore FPU registers on the stack. |
| `VMRS Rd, FPSCR` | Move from FP Status Register | Read the FPU status and control register. |
| `VMSR FPSCR, Rn` | Move to FP Status Register | Write the FPU status and control register. |

---

## 17. System, hints, and barriers

| Instruction | Name | Action |
| --- | --- | --- |
| `NOP` | No Operation | Do nothing. Do not use it for timing delays. |
| `WFI` | Wait For Interrupt | Stop the core until an interrupt arrives. It saves power. |
| `WFE` | Wait For Event | Stop the core until an event or an interrupt arrives. |
| `SEV` | Send Event | Signal an event. |
| `YIELD` | Yield | Tell a multi-threading system that this thread can give up the core. |
| `SVC #imm` | Supervisor Call | Cause an SVCall exception. An RTOS uses it for system calls. |
| `BKPT #imm` | Breakpoint | Stop in the debugger. |
| `DBG #imm` | Debug Hint | Give a hint to the debug system. |
| `DMB` | Data Memory Barrier | All memory accesses before it must complete before the accesses after it start. |
| `DSB` | Data Synchronization Barrier | All memory accesses before it must complete before any later instruction runs. |
| `ISB` | Instruction Synchronization Barrier | Flush the pipeline. Use it after you change `CONTROL`, the MPU, or the vector table. |
| `CPSID i` / `CPSID f` | Change Processor State, Disable | Set `PRIMASK` or `FAULTMASK`. It disables interrupts. |
| `CPSIE i` / `CPSIE f` | Change Processor State, Enable | Clear the mask. It enables interrupts. |

---

## 18. Register model

### Core registers

| Register | Alias | Use |
| --- | --- | --- |
| `R0`–`R3` | — | Arguments and scratch. The callee can change them (AAPCS). |
| `R4`–`R11` | — | Local variables. The callee must save them (AAPCS). |
| `R12` | `IP` | Intra-procedure scratch. |
| `R13` | `SP` | Stack pointer. There are two banked copies: `MSP` and `PSP`. |
| `R14` | `LR` | Link register. It holds the return address. |
| `R15` | `PC` | Program counter. |

### Special registers

| Register | Use |
| --- | --- |
| `APSR` | Application flags: N, Z, C, V, Q, and the GE bits for SIMD. |
| `IPSR` | The number of the active exception. |
| `EPSR` | Execution state, including the Thumb bit and the `IT` state. |
| `xPSR` | The combined view of `APSR`, `IPSR`, and `EPSR`. |
| `PRIMASK` | Set it to disable all interrupts with a configurable priority. |
| `FAULTMASK` | Set it to disable all interrupts and faults, except NMI. |
| `BASEPRI` | Disable all interrupts with a priority number equal to or above this value. |
| `CONTROL` | Selects the stack (`MSP` or `PSP`), the privilege level, and the FPU state. |
| `FPSCR` | FPU status and control (M4F only). |

### APSR flags

| Flag | Name | Set when |
| --- | --- | --- |
| `N` | Negative | Bit 31 of the result is 1. |
| `Z` | Zero | The result is zero. |
| `C` | Carry | An unsigned overflow or a shift carry-out occurred. |
| `V` | Overflow | A signed overflow occurred. |
| `Q` | Sticky Saturation | A saturating instruction clamped a result. Software must clear it. |
| `GE[3:0]` | Greater or Equal | Per-lane result of a SIMD operation. `SEL` reads these bits. |
