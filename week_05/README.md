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
