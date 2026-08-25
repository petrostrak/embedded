# Week 5

## Concepts

- [ ] **Ohm's law.** V = IR. Rearrange to all three forms from memory, no notes.
- [ ] **Series and parallel.** Combine resistances both ways.
  - [ ] Series: sum. Verify on the bench with two resistors and a meter.
  - [ ] Parallel: reciprocal sum. Verify on the bench.
  - [ ] Write down which one always gives a result smaller than the smallest resistor, and why.
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

## Project — Five circuits, measured

### Setup

- [ ] Breadboard, jumpers, resistor assortment, LED, tactile button, potentiometer.
- [ ] Multimeter. Confirm you can select voltage mode and current mode, and know which one goes in series.
- [ ] Logic analyzer connected, sample rate configurable, capture export working.
- [ ] Common ground between board, breadboard, meter, and analyzer. Check this first every time.

### Circuit 1 — LED + resistor

- [ ] Calculate the resistor for 5 mA at 3.3 V. Show the arithmetic.
- [ ] Pick the nearest real resistor value you own. Note which direction it moves the current.
- [ ] Build it.
- [ ] Measure the actual current through the LED.
- [ ] Measure the actual voltage drop across the LED, and across the resistor.
- [ ] Check the two drops sum to the supply. If they don't, find the error before continuing.
- [ ] Explain the discrepancy between calculated and measured current. Write it down.
- [ ] Swap in a resistor roughly 10× larger. Measure again. Note whether the LED voltage moved as much as the current did.

### Circuit 2 — Voltage divider

- [ ] Choose two resistors for 3.3 V → ~1.65 V. Note why the ratio matters and the absolute values don't, yet.
- [ ] Build it. Measure the unloaded output.
- [ ] Load the output with 1 kΩ. Measure again.
- [ ] Explain the sag. Write it down.
- [ ] Calculate what the loaded output should have been. Compare to measured.
- [ ] Edge case: short the output to ground. Predict the reading first, then measure.
- [ ] Optional: rebuild the same ratio with resistors 10× larger, reload with the same 1 kΩ, and write down how the sag changed.

### Circuit 3 — Button with external pull-up

- [ ] Wire button to ground, pull-up resistor from pin to 3.3 V.
- [ ] Measure the pin voltage released.
- [ ] Measure the pin voltage pressed.
- [ ] Compare both readings against the V_IH / V_IL numbers you wrote down. Confirm each sits in the right region.
- [ ] Remove the pull-up. Leave the pin otherwise untouched.
- [ ] Measure the floating pin. Move your hand near the wire and measure again.
- [ ] Write down what you observed and why the reading is not a valid logic level.
- [ ] Confirm the STM32F3's internal pull-up is disabled for this pin, so you are measuring what you think you are measuring.

### Circuit 4 — Potentiometer as adjustable divider

- [ ] Wire the pot as a divider: ends to 3.3 V and ground, wiper as output.
- [ ] Sweep the full range. Confirm the wiper reaches 0 V at one stop and 3.3 V at the other.
- [ ] Edge case: measure at both hard stops, not just near them.
- [ ] Measure at mid-travel. Note whether it is actually half, and whether the pot is linear or log taper.
- [ ] Load the wiper with 1 kΩ at mid-travel. Note the sag and compare it to Circuit 2.
- [ ] Note the wiper resistance changes with position — write down what that means for source impedance into an ADC.
- [ ] Keep this wiring. It becomes the Week 18 ADC input.

### Circuit 5 — Bounce capture

- [ ] Reuse the Circuit 3 button, pull-up back in place.
- [ ] Set the logic analyzer to its maximum sample rate.
- [ ] Note the sample interval at that rate. Confirm it is short enough to resolve what you are about to look at.
- [ ] Capture a single press.
- [ ] Count the transitions in the bounce burst.
- [ ] Measure the burst duration in milliseconds.
- [ ] Capture the release as well — bounce is not only a press-time event.
- [ ] Repeat over ~10 presses. Record the worst-case duration, not the typical one.
- [ ] Edge case: a slow, deliberate press. Compare to a fast tap.
- [ ] Save and export the capture to a file you can find again.

## Done when

- [ ] You have a saved logic-analyzer capture of real contact bounce, exported to disk, not just on screen.
- [ ] You have one number in milliseconds: the worst-case bounce duration you measured. Keep this number and the capture — it is your debounce interval in Week 14.
- [ ] You have kept the potentiometer divider wiring and its notes — it is the ADC input in Week 18.
- [ ] You can explain out loud, from memory: why the measured LED current missed the calculated 5 mA, why the divider sagged under load, and what a floating input actually reads.
- [ ] You can explain from memory why I²C needs pull-ups, in terms of what an open-drain output can and cannot drive.

<details>
<summary>Ohm's law</summary>

# Ohm's Law in Embedded C — Reference Note

## The three forms

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

Firmware often has no FPU. Scaled integers are faster and safe.
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
