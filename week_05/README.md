# Week 5

## Concepts

- [x] **Ohm's law.** V = IR. Rearrange to all three forms from memory, no notes.
- [x] **Series and parallel.** Combine resistances both ways.
  - [x] Series: sum. Verify on the bench with two resistors and a meter.
  - [x] Parallel: reciprocal sum. Verify on the bench.
  - [x] Write down which one always gives a result smaller than the smallest resistor, and why.
- [ ] **Voltage divider.** Derive the output formula from Ohm's law yourself; do not copy it.
- [ ] **Divider under load.** A load resistance sits in parallel with the bottom leg. Write down what that does to the output before you measure it.
- [ ] **LED current-limiting resistor.** Calculate the value from supply, forward voltage, and target current. Calculate, don't copy.
- [ ] **LED forward voltage.** Not a resistance. Find the forward voltage of the LED you are actually using.
- [ ] **Pull-up vs pull-down.**
  - [ ] Write down the resting logic level each one gives, and what the button then reads as when pressed.
  - [ ] Write down why an input with nothing connected reads garbage.
  - [ ] Note the STM32F3 has internal pull-ups and pull-downs per pin. Note why this week uses an external one anyway.
- [ ] **Push-pull vs open-drain.** Write down what each output stage can drive high, drive low, or not drive at all.
- [ ] **Why I²C needs pull-ups.** Follow it from the open-drain answer above. Write it down in one sentence.
- [ ] **3.3 V logic levels.** Find V_IH and V_IL for the STM32F303VC in the datasheet.
  - [ ] Write down both numbers.
  - [ ] Write down what the chip does with a voltage in the band between them.
- [ ] **5 V tolerance.** The F3DISCOVERY I/O is 3.3 V. Some pins are 5 V-tolerant, some are not — the `FT` designation in the datasheet pin table tells you which. Check before you connect anything 5 V.
- [ ] **Decoupling capacitors.** What they supply, and where they must be placed relative to the pin.
- [ ] **Reading a trace.**
  - [ ] Rise time — what you measure between, and why it isn't zero.
  - [ ] Ringing — what it looks like and where it comes from.
  - [ ] Bounce — mechanical, not electrical. Distinguish it from ringing on a capture.

## Done when

- [ ] You have a saved logic-analyzer capture of real contact bounce, exported to disk, not just on screen.
- [ ] You have one number in milliseconds: the worst-case bounce duration you measured. Keep this number and the capture — it is your debounce interval in Week 14.
- [ ] You have kept the potentiometer divider wiring and its notes — it is the ADC input in Week 18.
- [ ] You can explain out loud, from memory: why the measured LED current missed the calculated 5 mA, why the divider sagged under load, and what a floating input actually reads.
- [ ] You can explain from memory why I²C needs pull-ups, in terms of what an open-drain output can and cannot drive.

<details>
<summary>Ohm's law</summary>

Ohm's law states that the current through a conductor between two points is directly proportional to the potential difference across the two points. 

$I = \frac{V}{R}$

> R είναι η τιμή της αντίστασης σε μονάδες Ohm (Ω).  
> V είναι η διαφορά δυναμικού στα άκρα της αντίστασης, σε μονάδες Volt (V).  
> I είναι το ρεύμα το οποίο διαρρέει την αντίσταση μετρημένο σε Ampère (A).

## Related formula: power

Power comes from Ohm's law and `P = V * I`.

| Form | Use |
|---|---|
| `P = V * I` | Both values are known. |
| `P = I * I * R` | Current through a resistor is known. Use for shunt and trace heating. |
| `P = V * V / R` | Voltage across a resistor is known. Use for pull-up and divider loss. |

## Units: the rule that removes floating point

Firmware often has no FPU (floating-point unit). Scaled integers are faster and safe.
Select the units so that the scale factors cancel.

| Identity | Example |
|---|---|
| `Ω = mV / mA` | 3300 mV / 10 mA = 330 Ω |
| `mV = mA * Ω` | 10 mA x 330 Ω = 3300 mV |
| `mA = mV / Ω` | 3300 mV / 330 Ω = 10 mA |
| `mA = µV / mΩ` | 5000 µV / 100 mΩ = 50 mA |
| `µW = mV * mA` | 2000 mV x 10 mA = 20000 µW = 20 mW |

Keep the unit in the variable name. This prevents most scale errors.

```c
uint32_t v_mv;      /* millivolts */
uint32_t i_ma;      /* milliamps  */
uint32_t r_ohm;     /* ohms       */
uint32_t r_mohm;    /* milliohms  */
```

## Example: LED series resistor

Find the resistor value. Rearranged form: `R = V / I`.
The voltage across the resistor is the supply voltage minus the LED forward voltage.

```c
#include <stdint.h>

/* R = (Vsupply - Vf) / I   ->   Ω = mV / mA */
uint32_t led_resistor_ohm(uint32_t vsupply_mv, uint32_t vf_mv, uint32_t i_ma)
{
    if (i_ma == 0U || vsupply_mv <= vf_mv) {
        return 0U;                      /* invalid input */
    }
    return (vsupply_mv - vf_mv) / i_ma;
}

/* Example: 3300 mV supply, red LED Vf = 1800 mV, target 5 mA
 * (3300 - 1800) / 5 = 300 Ω  ->  select 330 Ω from the E12 series
 */
```

## Example: rounded integer division

Integer division truncates. Add half of the divisor to round.

```c
/* Positive values only. */
static inline uint32_t div_round_u32(uint32_t num, uint32_t den)
{
    return (num + den / 2U) / den;
}

/* (1500 + 165) / 330 = 5 mA  instead of 4 mA */
```

## Example: ADC counts to voltage, then to current

Two steps. First scale the ADC result. Then apply `I = V / R`.

```c
#define ADC_MAX_COUNTS  4095U      /* 12-bit ADC */
#define ADC_VREF_MV     3300U

/* Step 1: counts -> millivolts */
uint32_t adc_to_mv(uint16_t counts)
{
    /* 4095 * 3300 = 13513500. This exceeds 16 bits.
     * Use uint32_t for the product to prevent overflow.
     */
    return ((uint32_t)counts * ADC_VREF_MV) / ADC_MAX_COUNTS;
}

/* Step 2: I = V / R across a known load */
uint32_t load_current_ma(uint16_t counts, uint32_t r_load_ohm)
{
    return adc_to_mv(counts) / r_load_ohm;
}
```

**Overflow warning.** On an 8-bit or 16-bit target, `int` is 16 bits.
The product `counts * 3300` overflows. Always cast one operand to `uint32_t`
before you multiply.

## Example: high-side current sense with a shunt

The shunt is a small, known resistance. Measure the voltage across it.
Rearranged form: `I = V / R`.

```c
#define SHUNT_MOHM      100U       /* 0.1 Ω shunt */
#define AMP_GAIN        50U        /* current-sense amplifier gain */

/* mA = µV / mΩ */
uint32_t shunt_current_ma(uint32_t adc_mv)
{
    uint32_t v_sense_uv = (adc_mv * 1000U) / AMP_GAIN;  /* remove the gain */
    return v_sense_uv / SHUNT_MOHM;
}

/* Example: ADC reads 250 mV.
 * 250000 µV / 50 = 5000 µV across the shunt.
 * 5000 µV / 100 mΩ = 50 mA
 */
```

Check the shunt power rating with `P = I * I * R`:

```c
/* 50 mA through 0.1 Ω: P = 0.05 * 0.05 * 0.1 = 250 µW. A 0603 part is enough. */
uint32_t shunt_power_uw(uint32_t i_ma, uint32_t r_mohm)
{
    return (i_ma * i_ma * r_mohm) / 1000U;   /* mA * mA * mΩ = nW, /1000 -> µW */
}
```

## Example: voltage divider

The divider is Ohm's law applied two times.
The same current flows through both resistors:

```
I = Vin / (R1 + R2)          /* I = V / R */
Vout = I * R2                /* V = I * R */
Vout = Vin * R2 / (R1 + R2)
```

```c
/* Battery monitor: 100k over 100k divides the input by 2. */
uint32_t battery_mv(uint16_t counts, uint32_t r1_ohm, uint32_t r2_ohm)
{
    uint32_t node_mv = adc_to_mv(counts);
    return (node_mv * (r1_ohm + r2_ohm)) / r2_ohm;
}
```

Divide the resistors down to a small ratio before you multiply.
`node_mv * 200000` overflows a 32-bit variable if `node_mv` is large.
Use `(1U, 1U)` for a 1:1 divider, or use 64-bit math.

## Example: NTC thermistor resistance

The unknown value is the resistance. Rearranged form: `R = V / I`.
Circuit: `Vin — R_fixed — node — R_ntc — GND`.

```
I = (Vin - Vnode) / R_fixed        /* I = V / R  through the fixed resistor */
R_ntc = Vnode / I                  /* R = V / I  across the NTC */
R_ntc = R_fixed * Vnode / (Vin - Vnode)
```

```c
uint32_t ntc_resistance_ohm(uint32_t vnode_mv, uint32_t vin_mv, uint32_t r_fixed_ohm)
{
    if (vnode_mv == 0U || vnode_mv >= vin_mv) {
        return 0U;                  /* open or short circuit */
    }
    return (r_fixed_ohm * vnode_mv) / (vin_mv - vnode_mv);
}
```

The resistance to temperature step is a separate calculation.
Use a lookup table or the Steinhart-Hart equation. Ohm's law stops here.

## Example: pull-up resistor sink current

An open-drain output pulls the line to ground. The pull-up resistor sets the current.

```c
/* I = V / R.  4700 Ω pull-up on 3300 mV. */
uint32_t pullup_sink_ma = 3300U / 4700U;       /* = 0 mA -> truncation hides the value */
uint32_t pullup_sink_ua = (3300U * 1000U) / 4700U;  /* = 702 µA. Better unit. */
```

Select the unit so that the result is not smaller than 1.
Otherwise integer truncation gives zero.

## Common errors

| Error | Result | Prevention |
|---|---|---|
| Mixed units in one equation | Value is wrong by 1000x | Put the unit in the variable name. |
| 16-bit product on a small MCU | Overflow, wrapped value | Cast to `uint32_t` before you multiply. |
| Division before multiplication | Loss of resolution | Multiply first, then divide. |
| Division by zero | Undefined behavior, hard fault | Test the divisor before each division. |
| Truncation to zero | Silent zero reading | Use a smaller unit or round. |
| `float` on a target with no FPU | Slow code, large image | Use scaled integers. |
| Ohm's law on a non-linear part | Wrong value | An LED, diode or transistor is not a resistor. |

## Recall drill

Write the answers. Then check them below.

1. Give the three forms of Ohm's law.
2. A 220 Ω resistor has 3300 mV across it. What is the current?
3. You must limit the current to 20 mA from a 5000 mV supply. What is the resistance?
4. 12 mA flows through 47 Ω. What is the voltage?
5. Which C data type do you need for `4095 * 3300`? Why?
6. Give the three forms of the power equation.

**Answers**

1. `V = I * R`, `I = V / R`, `R = V / I`
2. `3300 / 220 = 15 mA`
3. `5000 / 20 = 250 Ω`
4. `12 * 47 = 564 mV`
5. `uint32_t`. The product is 13513500. This is larger than the 16-bit maximum of 65535.
6. `P = V * I`, `P = I * I * R`, `P = V * V / R`
</details>

<details>
<summary>Series and parallel</summary>

## The two rules

| Connection | Formula | Shared quantity | Quantity that adds |
|---|---|---|---|
| Series | `Rt = R1 + R2 + ... + Rn` | Current is the same in each resistor. | Voltages add. |
| Parallel | `1 / Rt = 1/R1 + 1/R2 + ... + 1/Rn` | Voltage is the same across each resistor. | Currents add. |

### Series

```
  ---[ R1 ]---[ R2 ]---
```

One path only. The same current must pass through both parts.
Each part drops its own voltage. Apply Ohm's law two times and add:

```
Vt = V1 + V2 = I*R1 + I*R2 = I*(R1 + R2)
Rt = Vt / I  = R1 + R2
```

### Parallel

```
      +---[ R1 ]---+
  ----+            +----
      +---[ R2 ]---+
```

Two paths. Both ends of each resistor are at the same two nodes,
so both see the same voltage. The currents add:

```
It = I1 + I2 = V/R1 + V/R2 = V*(1/R1 + 1/R2)
1/Rt = It / V = 1/R1 + 1/R2
```

### Short forms for parallel

| Case | Formula | Example |
|---|---|---|
| Two resistors | `Rt = (R1 * R2) / (R1 + R2)` | 330 ∥ 470 = 155100 / 800 = 193.9 Ω |
| N equal resistors | `Rt = R / N` | 4 x 100 Ω = 25 Ω |
| One resistor much larger | `Rt ≈ smaller value` | 100 Ω ∥ 100 kΩ = 99.9 Ω |

Note: `(R1 * R2) / (R1 + R2)` is only valid for **two** resistors.
For three or more, fold the pairs one at a time, or use the reciprocal sum.

## Which connection always gives less than the smallest resistor?

**Parallel.** The result is always smaller than the smallest resistor in the group.
Series is always larger than the largest resistor.

### Why — the algebra

Take the two-resistor form and group it:

```
Rt = (R1 * R2) / (R1 + R2) = R1 * [ R2 / (R1 + R2) ]
```

`R2 / (R1 + R2)` is a fraction that is always less than 1, because the
denominator holds `R2` plus a positive value. So `Rt < R1`.
Group the same expression the other way and you get `Rt < R2`.
Therefore `Rt` is below both values.

### Why — the physical reason

A resistor is a restriction on current. When you add a second path,
you give the current one more way to pass. The total current rises
for the same voltage. From `R = V / I`, a larger `I` at a fixed `V`
means a smaller `R`.

You can never make the flow harder by opening one more path.

### Why — conductance makes it obvious

Conductance is `G = 1 / R`, in siemens. The parallel rule is a plain sum:

```
Gt = G1 + G2 + ... + Gn
```

A sum of positive values is larger than its largest term.
So `Gt > Gmax`, and `Rt = 1 / Gt < Rmin`. The result follows directly.

### Useful bounds

For two resistors, the answer is trapped in a narrow range:

```
Rmin / 2  <=  Rt  <  Rmin
```

The lower limit occurs when both resistors are equal.
Use this to check your arithmetic in one second. For 330 ∥ 470,
the answer must sit between 165 Ω and 330 Ω. 193.9 Ω passes the check.

## Bench verification

Use two resistors with different values. 330 Ω and 470 Ω are good choices.
Both are large enough to ignore the meter leads, and small enough to be stable.

### Step 0 — Prepare the meter

1. Select the resistance range. Use autorange if it is available.
2. Touch the two probes together. Read the value.
   This is the lead resistance, normally 0.2 Ω to 0.5 Ω.
   Subtract it from every measurement, or use the REL/ZERO button.
3. Never measure resistance in a powered circuit. Remove the supply first
   and let the capacitors discharge. A live circuit gives a false reading and
   can damage the meter.
4. Hold the resistor by the body, not the leads. Your fingers add a parallel path.

### Step 1 — Measure each resistor alone

| Part | Marked | Tolerance | Allowed range | Measured |
|---|---|---|---|---|
| R1 | 330 Ω | 5% | 313.5 to 346.5 Ω | ______ |
| R2 | 470 Ω | 5% | 446.5 to 493.5 Ω | ______ |

Record the measured values. Use these, not the marked values, in the checks below.

### Step 2 — Series

Connect the resistors end to end on a breadboard.
Measure across the two outer ends.

```
Predicted: Rt = R1 + R2 = 330 + 470 = 800 Ω
Allowed range with 5% parts: 760 to 840 Ω
```

Compare against the sum of your **measured** values. Agreement should be
inside 1%. A larger error points to a bad breadboard contact.

Also confirm the rule: the result is above 470 Ω, the larger part.

### Step 3 — Parallel

Connect both resistors between the same two breadboard rows.
Measure across those two rows.

```
Predicted: Rt = (330 * 470) / (330 + 470) = 155100 / 800 = 193.9 Ω
Allowed range with 5% parts: 184.2 to 203.6 Ω
```

Confirm the rule: the result is below 330 Ω, the smaller part.
Also confirm the bound: it is above 165 Ω, which is half of the smaller part.

### Step 4 — Optional extra check

Use two equal resistors, for example 1 kΩ and 1 kΩ.

- Series must read near 2 kΩ.
- Parallel must read near 500 Ω, which is exactly half.

This is the fastest way to prove that you wired the parallel connection
and not the series connection.

### Troubleshooting the bench result

| Symptom | Probable cause |
|---|---|
| Reading is unstable or drifts | Bad breadboard contact, or a hand on the leads. |
| Parallel reads the same as one resistor | One resistor is not connected. Check both rows. |
| Series reads much too low | The resistors share a row and are in parallel. |
| Small values read high by 0.3 Ω | Lead resistance is not subtracted. |
| Reading is far outside tolerance | Wrong colour code read, or the part is damaged. |

## Firmware code

```c
#include <stdint.h>
#include <stddef.h>

/* Series: a plain sum. Overflow is the only risk. */
uint32_t r_series_ohm(const uint32_t *r, size_t n)
{
    uint64_t sum = 0U;
    for (size_t i = 0U; i < n; i++) {
        sum += r[i];
    }
    return (sum > UINT32_MAX) ? UINT32_MAX : (uint32_t)sum;
}

/* Parallel, two resistors. Rounded. */
uint32_t r_parallel_ohm(uint32_t r1, uint32_t r2)
{
    if (r1 == 0U || r2 == 0U) {
        return 0U;                          /* a short circuit wins */
    }
    /* 1 MΩ x 1 MΩ = 1e12. This overflows uint32_t. Use 64-bit for the product. */
    uint64_t num = (uint64_t)r1 * (uint64_t)r2;
    uint32_t den = r1 + r2;                 /* safe: both are below 2^31 in practice */
    return (uint32_t)((num + (den / 2U)) / den);
}

/* Parallel, N resistors. Fold the pairs. */
uint32_t r_parallel_n_ohm(const uint32_t *r, size_t n)
{
    if (n == 0U) {
        return 0U;
    }
    uint32_t acc = r[0];
    for (size_t i = 1U; i < n; i++) {
        acc = r_parallel_ohm(acc, r[i]);
    }
    return acc;
}
```

**Rounding note.** Pairwise folding rounds at each step.
For three or more resistors, work in milliohms, or accept an error of
a few ohms. For a self test that only needs a pass or fail decision,
compare against a window instead of an exact value.

### Assertion for a design check

```c
/* Use the bound as a cheap unit test. */
#include <assert.h>

void test_parallel_bound(void)
{
    uint32_t rt = r_parallel_ohm(330U, 470U);
    assert(rt < 330U);          /* parallel is below the smallest part */
    assert(rt >= 330U / 2U);    /* and at or above half of it          */
}
```

## Where this appears in firmware and on the board

| Case | What happens |
|---|---|
| I2C pull-ups on two stacked boards | Two 4.7 kΩ pull-ups in parallel give 2.35 kΩ. The bus rise time changes and the sink current doubles. Fit the pull-up on one board only. |
| Shunt resistors in parallel | Two 200 mΩ parts give 100 mΩ and double the power rating. Recalculate the current scale factor in the firmware. |
| ADC source impedance | The ADC sees the divider legs in parallel, `R1 ∥ R2`. Keep this below the value in the datasheet, or the sample capacitor cannot charge in time. |
| Extra load on a divider | Any load resistance appears in parallel with the lower leg. The divider output falls. |
| Series resistors for a high voltage | Series parts split the voltage, so each part stays inside its own voltage rating. |
| Resistance to reach a value you cannot buy | Series adds, parallel reduces. Two 1 kΩ parts give 2 kΩ or 500 Ω. |

## Recall drill

Write the answers, then check below.

1. Give the series formula and the parallel formula.
2. Which connection always gives less than the smallest resistor? State the reason in one sentence.
3. 330 Ω and 470 Ω in series. Value?
4. 330 Ω and 470 Ω in parallel. Value?
5. Five 100 Ω resistors in parallel. Value?
6. Two 4.7 kΩ pull-ups on the same line. Value?
7. Why does `(R1 * R2) / (R1 + R2)` need a 64-bit product in C?
8. Without a calculator, give the range that 220 ∥ 680 must fall inside.

**Answers**

1. `Rt = R1 + R2 + ...` and `1/Rt = 1/R1 + 1/R2 + ...`
2. Parallel. Each added path lets more current pass for the same voltage, and `R = V / I` falls when `I` rises.
3. 800 Ω
4. 193.9 Ω
5. 20 Ω
6. 2.35 kΩ
7. Two large values, for example 1 MΩ each, give a product of 1e12. This is above the 32-bit limit of about 4.29e9.
8. Between 110 Ω and 220 Ω. The true value is 166.4 Ω.
</details>

<details>
<summary>Voltage divider</summary>

A voltage divider is two resistors in series across a voltage source. You read the voltage at the point between them. That point always has a lower voltage than the source.

```
        Vin
         |
        [ ]  R1
         |
         +------> Vout  (to ADC pin)
         |
        [ ]  R2
         |
        GND
```

In firmware you use it for one of two jobs:

1. **Scale a voltage down** so it fits inside the ADC input range. Example: read a 12 V battery with a 3.3 V microcontroller.
2. **Measure a resistance** by putting the unknown resistor in one of the two positions. Example: NTC thermistor, LDR, potentiometer.

## Derivation of the output formula

Start with two facts.

**Fact 1 — Ohm's law.** The voltage across a resistor is the current through it multiplied by its resistance.

```
V = I × R
```

**Fact 2 — Kirchhoff's voltage law.** The two resistor voltages must add up to the source voltage.

```
Vin = V_R1 + V_R2
```

### Step 1: The current is the same in both resistors

R1 and R2 are in series. There is only one path. Assume no current leaves the middle node (an ADC pin draws almost none). Therefore the same current `I` flows through R1 and through R2.

### Step 2: Write each resistor voltage with Ohm's law

```
V_R1 = I × R1
V_R2 = I × R2
```

### Step 3: Substitute into Kirchhoff's law

```
Vin = (I × R1) + (I × R2)
Vin = I × (R1 + R2)
```

### Step 4: Solve for the current

```
I = Vin / (R1 + R2)
```

### Step 5: Vout is the voltage across R2

Vout is measured from the middle node to ground. That is exactly the voltage across R2.

```
Vout = I × R2
```

### Step 6: Replace I with the result of step 4

```
             Vin
Vout  =  ------------- × R2
          (R1 + R2)
```

**Final form:**

```
Vout = Vin × R2 / (R1 + R2)
```

### Step 7: Invert it for firmware

Firmware does the opposite job. It knows Vout (the ADC measured it) and must find Vin. Multiply both sides by `(R1 + R2)` and divide by `R2`:

```
Vin = Vout × (R1 + R2) / R2
```

Call the term `(R1 + R2) / R2` the **divider ratio**. It is a fixed number set by your hardware.

### Sanity check with real numbers

Take Vin = 12 V, R1 = 100 kΩ, R2 = 20 kΩ.

- Current: `I = 12 / 120000 = 100 µA`
- Voltage across R2: `V = 100 µA × 20 kΩ = 2.0 V`
- Formula: `12 × 20000 / 120000 = 2.0 V` ✔

The divider ratio is `120000 / 20000 = 6`. A 2.0 V reading means 12 V at the input.

## Design rules before you write code

### Design for the maximum input, not the nominal input

The ADC pin must never go above the reference voltage. A "12 V" lead-acid battery reaches 14.4 V during charge.

- Worst case at the pin: `14.4 / 6 = 2.4 V`. Safe under 3.3 V.
- Full-scale input of this divider: `3.3 × 6 = 19.8 V`.

### Keep the source resistance low enough for the ADC

The ADC sees the two resistors in parallel (the Thevenin equivalent):

```
R_source = (R1 × R2) / (R1 + R2)
```

For 100 kΩ and 20 kΩ this is 16.7 kΩ. Many microcontrollers specify a maximum of about 10 kΩ. Above that limit the internal sample capacitor does not charge fully and readings come out low.

Three ways to fix it:

| Fix | Effect | Cost |
|---|---|---|
| Use smaller resistors (10 kΩ / 2 kΩ) | R_source drops to 1.67 kΩ | Current rises to 1 mA — bad for battery devices |
| Add 100 nF from the node to ground | The cap supplies the charge to the ADC | Slow step response; do not use on fast multiplexed channels |
| Increase the ADC sample time | No extra parts | Slower conversions |

### Watch the quiescent current

```
I = Vin / (R1 + R2)
```

100 kΩ + 20 kΩ at 12 V draws 100 µA, all day, forever. On a coin cell that is fatal. Options: use MΩ resistors with an op-amp buffer, or switch the bottom of the divider to ground with a MOSFET and only enable it during a measurement.

### Accept the tolerance error

Two 1 % resistors give a ratio error of up to about 2 %. Your options are 0.1 % parts, or a one-point calibration: apply a known input, measure it, store the correction factor in non-volatile memory.

### A divider is not a level shifter

For a slow one-way digital signal (5 V transmitter → 3.3 V receiver) a divider works. For fast edges the RC formed by the resistors and the trace/pin capacitance rounds the signal off. For bidirectional buses such as I²C, use a proper level shifter.

## Firmware: converting ADC counts to volts

### The two conversions

```
node_voltage   = counts × Vref / ADC_FULL_SCALE
source_voltage = node_voltage × (R1 + R2) / R2
```

### Use integers, not floats

Most microcontrollers have no hardware floating point unit. Work in **millivolts** with `uint32_t`. Combine both conversions into one multiply and one divide. This keeps the rounding error to a single step.

```c
#include <stdint.h>

/* ---- Hardware constants -------------------------------------------- */
#define ADC_FULL_SCALE   4095u      /* 12-bit ADC: counts run 0..4095   */
#define VREF_MV          3300u      /* ADC reference voltage, millivolts */
#define DIV_R1_OHM       100000u    /* top resistor                      */
#define DIV_R2_OHM       20000u     /* bottom resistor, node is above it */

/* Divider ratio as a fraction. No floats, no rounding at compile time. */
#define DIV_NUM          (DIV_R1_OHM + DIV_R2_OHM)
#define DIV_DEN          (DIV_R2_OHM)

/* Input voltage that produces a full-scale ADC reading = 19800 mV. */
#define FULL_SCALE_MV    ((VREF_MV * DIV_NUM) / DIV_DEN)

/**
 * Convert a raw ADC reading into the voltage at the divider input.
 * Returns millivolts.
 */
uint32_t divider_counts_to_mv(uint16_t counts)
{
    /* Round to nearest instead of truncating: add half the divisor. */
    return ((uint32_t)counts * FULL_SCALE_MV + (ADC_FULL_SCALE / 2u))
           / ADC_FULL_SCALE;
}
```

**Overflow check.** Always do this by hand. The largest intermediate value is `4095 × 19800 = 81,081,000`. A `uint32_t` holds up to 4,294,967,295. There is plenty of margin. If your numbers were larger you would need `uint64_t`, or you would scale down first.

**Why the cast matters.** `counts` is a `uint16_t`. On a 32-bit target integer promotion saves you, but on an 8-bit or 16-bit target the product would wrap. The explicit `(uint32_t)` cast makes the code correct on every target.

### A reusable, table-driven version

Different channels have different dividers. Put the hardware description in a struct instead of in `#define`s.

```c
typedef struct {
    uint32_t r_top_ohm;
    uint32_t r_bottom_ohm;
    uint32_t vref_mv;
    uint16_t adc_full_scale;
} divider_cfg_t;

static const divider_cfg_t battery_div = {
    .r_top_ohm      = 100000u,
    .r_bottom_ohm   = 20000u,
    .vref_mv        = 3300u,
    .adc_full_scale = 4095u,
};

uint32_t divider_read_mv(const divider_cfg_t *cfg, uint16_t counts)
{
    uint32_t ratio_num = cfg->r_top_ohm + cfg->r_bottom_ohm;
    uint32_t fs_mv     = (cfg->vref_mv * ratio_num) / cfg->r_bottom_ohm;

    return ((uint32_t)counts * fs_mv + (cfg->adc_full_scale / 2u))
           / cfg->adc_full_scale;
}
```

### Correct the reference voltage at runtime

If your ADC reference is the supply rail, the supply sags as the battery drains and every reading drifts. Most microcontrollers have an internal bandgap channel for this. Measure it, calculate the true supply, then use that value as `vref_mv`.

```c
/* Bandgap value from the device datasheet, in millivolts. */
#define VBANDGAP_MV      1210u

uint32_t measure_vdda_mv(uint16_t bandgap_counts)
{
    if (bandgap_counts == 0u) {
        return VREF_MV;                 /* fall back to the nominal value */
    }
    return ((uint32_t)VBANDGAP_MV * ADC_FULL_SCALE) / bandgap_counts;
}
```

### Average the samples

A single ADC reading is noisy. Take 16 samples and shift right by 4. The divide becomes a shift, which is cheap.

```c
#define OVERSAMPLE_LOG2  4u
#define OVERSAMPLE_N     (1u << OVERSAMPLE_LOG2)

uint16_t adc_read_averaged(uint8_t channel)
{
    uint32_t sum = 0u;

    for (uint8_t i = 0u; i < OVERSAMPLE_N; i++) {
        sum += adc_read_raw(channel);
    }
    return (uint16_t)(sum >> OVERSAMPLE_LOG2);
}
```

## Example: battery monitor

Hardware: 12 V battery, R1 = 100 kΩ, R2 = 20 kΩ, 3.3 V reference, 12-bit ADC.

```c
#define BATT_LOW_MV      11400u   /* warn below this  */
#define BATT_CRIT_MV     10500u   /* shut down below this */

typedef enum {
    BATT_OK,
    BATT_LOW,
    BATT_CRITICAL
} batt_state_t;

batt_state_t battery_check(uint32_t *out_mv)
{
    uint16_t counts = adc_read_averaged(ADC_CH_BATTERY);
    uint32_t mv     = divider_counts_to_mv(counts);

    *out_mv = mv;

    if (mv < BATT_CRIT_MV) {
        return BATT_CRITICAL;
    }
    if (mv < BATT_LOW_MV) {
        return BATT_LOW;
    }
    return BATT_OK;
}
```

Add hysteresis before you act on the result. A motor start pulls the rail down for a few milliseconds and will trip a naive threshold.

## Example: NTC thermistor (ratiometric measurement)

Put the fixed resistor on top and the NTC on the bottom. Power the divider from the same rail that feeds the ADC reference.

```
       VDDA (= Vref)
         |
        [ ]  R_FIXED = 10 kΩ
         |
         +------> ADC
         |
        [ ]  NTC
         |
        GND
```

Now something useful happens. Start from the divider formula:

```
Vout = Vin × R_ntc / (R_FIXED + R_ntc)
```

The ADC reports `counts = ADC_FULL_SCALE × Vout / Vref`. Because `Vin` and `Vref` are the same rail, they cancel:

```
counts / ADC_FULL_SCALE = R_ntc / (R_FIXED + R_ntc)
```

Rearrange for the unknown resistance:

```
counts × (R_FIXED + R_ntc) = ADC_FULL_SCALE × R_ntc
counts × R_FIXED           = R_ntc × (ADC_FULL_SCALE − counts)

R_ntc = R_FIXED × counts / (ADC_FULL_SCALE − counts)
```

The supply voltage has disappeared from the equation. Supply drift no longer affects the temperature reading. This is called a **ratiometric** measurement, and it is the main reason to use this topology for sensors.

```c
#define R_FIXED_OHM      10000u

#define NTC_SHORTED      0u
#define NTC_OPEN         0xFFFFFFFFu

uint32_t ntc_resistance_ohm(uint16_t counts)
{
    if (counts == 0u) {
        return NTC_SHORTED;                 /* node stuck at ground */
    }
    if (counts >= ADC_FULL_SCALE) {
        return NTC_OPEN;                    /* NTC disconnected     */
    }
    /* Max intermediate: 10000 × 4094 = 40,940,000. Fits in uint32_t. */
    return ((uint32_t)R_FIXED_OHM * counts) / (ADC_FULL_SCALE - counts);
}
```

Convert resistance to temperature with a lookup table in flash. Do not use the Steinhart-Hart equation on a small microcontroller; it needs logarithms. A 16-entry table plus linear interpolation is accurate to a fraction of a degree and costs a few hundred bytes.

```c
typedef struct {
    uint32_t ohm;
    int16_t  temp_c10;      /* tenths of a degree Celsius */
} ntc_point_t;

/* Sorted by descending resistance (NTC: resistance falls as temp rises). */
static const ntc_point_t ntc_table[] = {
    { 32650u, -100 },   /* -10.0 °C */
    { 19900u,    0  },
    { 12490u,  100  },   /*  10.0 °C */
    {  8057u,  200  },
    {  5327u,  300  },
    {  3603u,  400  },
    {  2488u,  500  },
};
#define NTC_TABLE_LEN (sizeof(ntc_table) / sizeof(ntc_table[0]))

int16_t ntc_ohm_to_temp_c10(uint32_t ohm)
{
    if (ohm >= ntc_table[0].ohm) {
        return ntc_table[0].temp_c10;                    /* clamp cold end */
    }
    for (uint8_t i = 1u; i < NTC_TABLE_LEN; i++) {
        if (ohm >= ntc_table[i].ohm) {
            uint32_t span_ohm  = ntc_table[i - 1u].ohm - ntc_table[i].ohm;
            uint32_t into_span = ntc_table[i - 1u].ohm - ohm;
            int32_t  span_temp = ntc_table[i].temp_c10
                               - ntc_table[i - 1u].temp_c10;

            return (int16_t)(ntc_table[i - 1u].temp_c10
                   + ((int32_t)into_span * span_temp) / (int32_t)span_ohm);
        }
    }
    return ntc_table[NTC_TABLE_LEN - 1u].temp_c10;        /* clamp hot end */
}
```

## Example: potentiometer

A potentiometer is a voltage divider you can turn. The wiper splits the track into R1 and R2, and the two always add up to the total track resistance. Wire the ends across VDDA and ground and the wiper to the ADC. The reading is directly proportional to position, so no divider maths is needed at all.

```c
/* Map a raw reading to 0..100 percent. */
uint8_t pot_to_percent(uint16_t counts)
{
    return (uint8_t)(((uint32_t)counts * 100u + (ADC_FULL_SCALE / 2u))
                     / ADC_FULL_SCALE);
}
```

Add a dead band of a few counts at each end. A real potentiometer rarely reaches its true limits, and the last few counts are noisy.

## Testing without hardware

The conversion functions are pure integer maths. Compile them for your host machine and test them there.

```c
#include <assert.h>

void test_divider(void)
{
    /* 12.0 V input -> 2.0 V at the node -> 2482 counts at 3.3 V / 12 bit. */
    assert(divider_counts_to_mv(2482u) > 11950u);
    assert(divider_counts_to_mv(2482u) < 12050u);

    assert(divider_counts_to_mv(0u) == 0u);
    assert(divider_counts_to_mv(ADC_FULL_SCALE) == FULL_SCALE_MV);

    /* NTC at exactly half scale must read the fixed resistor value. */
    assert(ntc_resistance_ohm(2047u) > 9950u);
    assert(ntc_resistance_ohm(2047u) < 10050u);
}
```
</details>

<details>
<summary>LED current-limiting resistor and LED forward voltage</summary>

A resistor obeys Ohm's law. Double the voltage and you double the current. An LED does not behave that way.

An LED is a diode. Below a threshold voltage it conducts almost nothing. Above that threshold the current rises exponentially. A change of 0.1 V can multiply the current by ten.

```
 I
 |                              /|
 |                             / |   <-- almost vertical
 |                            /
 |                           /
 |__________________________/
 +--------------------------------- V
 0                        Vf
```

Two consequences:

1. You cannot set the current by setting the voltage. The slope is too steep to control.
2. There is nothing inside the LED to stop the current. Connect an LED directly across a supply and the current is limited only by the supply and the wiring. The LED overheats and fails, sometimes in under a second.

So you put a resistor in series. The resistor is the flat, predictable part of the circuit. It sets the current.

```
        Vsupply
          |
         [ ]  R      <-- sets the current
          |
          V  LED     <-- drops Vf, whatever the current
          -
          |
         GND
```

## Forward voltage (Vf)

**Forward voltage is the voltage an LED drops when it is conducting.** It is roughly constant over the normal operating range. It is set mainly by the semiconductor chemistry, which is why it tracks colour.

| Colour | Typical Vf at 20 mA |
|---|---|
| Infrared | 1.2 – 1.5 V |
| Red | 1.8 – 2.2 V |
| Amber / Yellow | 2.0 – 2.2 V |
| Green (standard) | 2.0 – 2.2 V |
| Green (pure / InGaN) | 3.0 – 3.4 V |
| Blue | 2.8 – 3.4 V |
| White | 2.8 – 3.4 V |

Four things move Vf. All of them matter in a real design:

- **Current.** Vf rises slowly as current rises. A datasheet quotes Vf at one test current, often 20 mA. At 2 mA the same LED drops noticeably less.
- **Temperature.** Vf falls about 2 mV per °C of rise. An LED that warms up draws more current, which warms it further.
- **Part-to-part spread.** Manufacturers bin LEDs. A single part number can span 2.8 V to 3.4 V.
- **Chemistry.** Never assume a replacement part has the same Vf, even in the same colour.

**Rule:** design with the datasheet minimum and maximum Vf, not the typical value. Then confirm on the bench with a real part.

## Derivation of the resistor formula

Start with two facts.

**Fact 1 — Kirchhoff's voltage law.** Around a series loop, the voltage drops add up to the source voltage.

**Fact 2 — Ohm's law.** `V = I × R`.

### Step 1: Write the loop

The supply drives a resistor and an LED in series.

```
Vsupply = V_R + Vf
```

### Step 2: Find the voltage across the resistor

The LED takes Vf. Whatever is left is across the resistor. Rearrange:

```
V_R = Vsupply − Vf
```

This leftover voltage is called the **headroom**. Remember the name; section 6 depends on it.

### Step 3: The current is the same in both parts

The resistor and the LED are in series. There is one path. The current through the resistor is the current through the LED. Call it `If`.

### Step 4: Apply Ohm's law to the resistor

```
V_R = If × R
```

### Step 5: Substitute step 2 into step 4

```
Vsupply − Vf = If × R
```

### Step 6: Solve for R

Divide both sides by `If`:

```
        Vsupply − Vf
R  =  ----------------
             If
```

**That is the design formula.** Only the resistor is involved in the maths. The LED contributes one number, Vf.

### Step 7: Invert it for verification

You will not get the exact resistor you calculated. Standard values come in fixed steps. So rearrange to find the current you will actually get from the part you can actually buy:

```
        Vsupply − Vf
If  =  ----------------
              R
```

### Worked example

Supply 3.3 V, red LED with Vf = 1.9 V, target current 5 mA.

- Headroom: `3.3 − 1.9 = 1.4 V`
- Ideal resistor: `1.4 / 0.005 = 280 Ω`
- Nearest standard value **above** 280 Ω in the E12 series: 330 Ω
- Actual current: `1.4 / 330 = 4.24 mA`

**Always round the resistor up.** A larger resistor gives less current. Less current is safe. More current is not.

## Power dissipation

Check both parts. A 0402 resistor is typically rated for 62.5 mW, a 0603 for 100 mW.

**In the resistor:**

```
P_R = If² × R          (or equivalently  P_R = (Vsupply − Vf) × If)
```

For the example above: `0.00424² × 330 = 5.9 mW`. Any package is fine.

**In the LED:**

```
P_LED = Vf × If
```

For the example: `1.9 × 0.00424 = 8.1 mW`.

**Efficiency.** The resistor burns the headroom as heat. The wasted fraction is `(Vsupply − Vf) / Vsupply`, which here is 42 %. At 5 mA nobody cares. At 350 mA that is 490 mW of heat, and you must use a constant-current driver instead of a resistor.

## Microcontroller pin limits

Three separate limits apply, and all three must hold.

1. **Per-pin current.** Typically 8 mA to 20 mA. Check the absolute maximum, then design well below it.
2. **Total port or package current.** Often around 100 mA for the whole device. Eight LEDs at 20 mA already breaks it.
3. **Output voltage drop.** A GPIO is not an ideal switch. It has internal resistance. The datasheet gives VOH (high output) and VOL (low output) at a stated current.

### Correct the supply for the pin drop

If the datasheet says VOL = 0.4 V at 8 mA, then a low-side driven LED does not see 3.3 V. It sees:

```
V_effective = 3.3 − 0.4 = 2.9 V
```

Use 2.9 V in the resistor formula, not 3.3 V. Skip this and your current will be lower than you designed for, and the LED will look dim.

### Sink or source

```
   SOURCE (active high)          SINK (active low)
                                        VDD
       GPIO                              |
        |                               [ ] R
       [ ] R                             |
        |                                V  LED
        V  LED                           -
        -                                |
        |                              GPIO
       GND
```

Many microcontrollers sink more current than they source. Sinking is often the stronger and cleaner option. It also means the LED is **on when the pin is low**. Wrap that inversion in a macro so it appears exactly once in your code:

```c
/* LED anode to VDD through R, cathode to the GPIO: on = pin low. */
#define LED_ON()    gpio_write(LED_PORT, LED_PIN, 0)
#define LED_OFF()   gpio_write(LED_PORT, LED_PIN, 1)
```

Set the pin to its off state **before** you configure it as an output. Otherwise the LED flashes during boot.

## How much headroom you need

This is where most LED designs go wrong.

Take the current formula and ask how much the current moves when Vf moves:

```
If = (Vsupply − Vf) / R
```

A change in Vf of `ΔVf` changes the numerator by the same amount. So the fractional change in current is:

```
ΔIf / If  =  ΔVf / (Vsupply − Vf)
```

The error in your current is the Vf spread **divided by the headroom**. Small headroom means large error.

### Compare two cases

**Red LED on 3.3 V.** Vf spread 1.8 – 2.0 V, so ΔVf = 0.2 V. Headroom ≈ 1.4 V.

```
0.2 / 1.4 = 14 % current spread.
```

Acceptable.

**Blue LED on 3.3 V.** Vf spread 2.8 – 3.4 V, so ΔVf = 0.6 V. Headroom at typical Vf ≈ 0.3 V.

```
0.6 / 0.3 = 200 % current spread.
```

Worse: an LED binned at 3.4 V has **negative** headroom on a 3.3 V rail that has sagged to 3.2 V. It will not light at all. Some boards will work and some will not, and the failures will look random.

**Rule of thumb: keep the headroom at or above 20 % of the supply, and above 0.5 V in absolute terms.** If you cannot, raise the supply (drive blue and white LEDs from 5 V), or use a constant-current sink.

## Multiple LEDs

### Series — good

Stack the forward voltages and use one resistor.

```
R = (Vsupply − Vf1 − Vf2 − ...) / If
```

Two red LEDs at 1.9 V need 3.8 V. On a 3.3 V rail they will not light. Check the sum against the supply, with headroom, before you commit.

### Parallel with one shared resistor — never do this

Two LEDs in parallel do not share the current evenly. The one with the lower Vf takes most of it. It gets hot, its Vf falls further, and it takes even more. This runs away. Brightness is mismatched and lifetime is short.

**Give every parallel LED its own resistor.** The cost of a resistor is far below the cost of a field failure.

### RGB LEDs

Each die is a different colour with a different Vf. Red might be 2.0 V while green and blue are 3.2 V. **Use three different resistor values**, calculated separately. Then trim them for white balance, because the three dies also differ in light output per milliamp.

## Firmware: encode the design in the code

Put the hardware numbers in the source. Let the compiler check them. Work in millivolts and microamps so everything stays in integers.

```c
#include <stdint.h>
#include <assert.h>

/* ---- Board constants ------------------------------------------------ */
#define VSUPPLY_MV        3300u
#define GPIO_VOL_MV        400u    /* pin drop when sinking, from datasheet */
#define VEFF_MV           (VSUPPLY_MV - GPIO_VOL_MV)

/* ---- Per-LED design ------------------------------------------------- */
#define LED_STATUS_VF_MV  1900u    /* red, datasheet typical */
#define LED_STATUS_R_OHM   330u    /* fitted part */

/* Actual current, in microamps.
 * mV / uA gives ohms, so mV * 1000 / ohms gives uA. */
#define LED_CURRENT_UA(vf_mv, r_ohm) \
    (((VEFF_MV - (vf_mv)) * 1000u) / (r_ohm))

#define LED_STATUS_IF_UA  LED_CURRENT_UA(LED_STATUS_VF_MV, LED_STATUS_R_OHM)

/* ---- Design rules, checked at compile time -------------------------- */
#define GPIO_MAX_UA       8000u
#define LED_MIN_VISIBLE_UA 1000u

_Static_assert(VEFF_MV > LED_STATUS_VF_MV,
               "No headroom: the LED will not light.");
_Static_assert(LED_STATUS_IF_UA <= GPIO_MAX_UA,
               "LED current exceeds the GPIO rating.");
_Static_assert(LED_STATUS_IF_UA >= LED_MIN_VISIBLE_UA,
               "LED current too low to see.");
```

Change the resistor in the schematic, change one number here, and the build tells you immediately if the design broke. No maths at runtime and no flash cost.

Add a budget check across the whole board:

```c
#define LED_TOTAL_UA  (LED_STATUS_IF_UA + LED_ERROR_IF_UA + LED_LINK_IF_UA)

_Static_assert(LED_TOTAL_UA <= 100000u,
               "Total LED current exceeds the package limit.");
```

## Brightness control with PWM

You cannot dim an LED usefully by changing the resistor at runtime, and lowering the current changes the colour of white and green LEDs. Switch it on and off fast instead. The eye averages the result.

Three rules:

- Keep the PWM frequency at 400 Hz or above. 100 Hz flickers visibly at the edge of vision and beats badly against camera shutters.
- The current during the on-time is still the full `If`. PWM changes the average, not the peak. Your resistor calculation does not change.
- Perceived brightness is not proportional to duty cycle.

### Gamma correction

The eye responds roughly logarithmically. A linear duty ramp looks wrong: it jumps at the bottom and then appears to stop changing. Apply a power curve.

A cheap approximation, accurate enough for indicators:

```c
/* Square law (gamma 2.0). Input 0..255, output 0..255. */
static inline uint8_t gamma_u8(uint8_t level)
{
    return (uint8_t)(((uint16_t)level * level) / 255u);
}
```

For a smooth fade on a 12-bit PWM timer, use a table with interpolation. Generate the table offline and store it in flash:

```c
/* 17 points of a gamma 2.2 curve, 8-bit input mapped to 12-bit duty. */
static const uint16_t gamma_lut[17] = {
       0,    1,    5,   14,   30,   55,   90,  137,
     197,  272,  363,  471,  598,  745,  913, 1103,
    4095
};

uint16_t gamma_to_duty(uint8_t level)
{
    uint8_t  idx  = (uint8_t)(level >> 4);        /* 0..15 */
    uint8_t  frac = (uint8_t)(level & 0x0Fu);     /* 0..15 */
    uint16_t lo   = gamma_lut[idx];
    uint16_t hi   = gamma_lut[idx + 1u];

    return (uint16_t)(lo + (((uint32_t)(hi - lo) * frac) >> 4));
}
```

### A non-blocking fade

Never use a delay loop to fade an LED. Drive it from a periodic tick.

```c
typedef struct {
    uint8_t  current;
    uint8_t  target;
    uint8_t  step;
} led_fade_t;

/* Call this at a fixed rate, for example every 10 ms. */
void led_fade_tick(led_fade_t *f)
{
    if (f->current < f->target) {
        uint8_t room = (uint8_t)(f->target - f->current);
        f->current = (uint8_t)(f->current + ((room < f->step) ? room : f->step));
    } else if (f->current > f->target) {
        uint8_t room = (uint8_t)(f->current - f->target);
        f->current = (uint8_t)(f->current - ((room < f->step) ? room : f->step));
    } else {
        return;                                   /* already there */
    }

    pwm_set_duty(LED_PWM_CHANNEL, gamma_to_duty(f->current));
}
```

## Multiplexed displays

In a scanned matrix or a 7-segment display, each LED is on for only a fraction of the frame. To keep the same apparent brightness you raise the peak current.

```
If_peak = If_average × N            (N = number of scanned rows)
```

Two hard limits apply:

- The LED datasheet gives a **peak pulse current** rating, valid only up to a maximum pulse width and duty cycle. Stay inside both.
- The GPIO or driver must carry the peak, not the average.

Guard the duty cycle in code, because a firmware bug that stops the scan leaves one row on at peak current permanently. That destroys the LEDs in seconds.

```c
#define MUX_ROWS            8u
#define MUX_FRAME_HZ      200u                     /* 200 Hz, no flicker */
#define MUX_ROW_US        (1000000u / (MUX_FRAME_HZ * MUX_ROWS))

/* Watchdog on the scanner: if the row timer stops, blank everything. */
void mux_isr(void)
{
    static uint8_t row = 0u;

    mux_all_rows_off();          /* blank first, then switch: no ghosting */
    mux_set_columns(frame_buffer[row]);
    mux_row_on(row);

    row = (uint8_t)((row + 1u) % MUX_ROWS);
    mux_watchdog_feed();
}
```

## Common mistakes

- **No resistor at all**, relying on the GPIO output resistance. The value is undefined, varies with temperature and part, and is not a specified limit.
- **Using the datasheet typical Vf only.** Design across the full bin range.
- **Driving blue or white LEDs from 3.3 V.** Not enough headroom. See section 6.
- **One resistor for parallel LEDs.** Uneven current and thermal runaway.
- **Ignoring the GPIO voltage drop.** Your real current is lower than calculated.
- **Forgetting the total package current limit** after checking each pin individually.
- **Reverse voltage.** LEDs typically survive only about 5 V in reverse. Charlieplexing and matrix schemes must respect this.
- **Configuring the GPIO before setting its level.** Causes a flash at reset.
- **Assuming a dimmer LED draws less peak current under PWM.** It does not.
</details>
