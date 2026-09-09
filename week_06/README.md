# Week 6 

## Concepts

- [ ] **Datasheet anatomy.** Know what lives in each part before you need it.
  - [ ] Absolute maximums — the numbers that destroy the part, not the numbers you design to.
  - [ ] Electrical characteristics — the numbers you design to. Note the test conditions attached to each.
  - [ ] Timing diagrams — how to read setup, hold, and propagation from the figure and its table.
  - [ ] Pinout and pin description tables — alternate functions, and the `FT` 5 V-tolerance column.
- [ ] **The four documents.** Write down, in one line each, which question you take to which document.
  - [ ] Datasheet — electricals, per-pin limits, package and pinout.
  - [ ] Reference manual (RM0316) — registers, bit fields, peripheral behaviour.
  - [ ] Board user manual — what is wired where on the F3DISCOVERY.
  - [ ] Schematic — ground truth. Overrides the user manual when they disagree.
- [ ] **Register tables.** Read one completely before you write to it.
  - [ ] Offset — from the peripheral base address, not absolute.
  - [ ] Reset value — what the bits are before your code runs.
  - [ ] Bit fields — width, position, and the encoding table underneath.
  - [ ] Access type: `rw`, `ro`, `rc_w1`. Write down what `rc_w1` means and why a naive read-modify-write is wrong on that kind of bit.
- [ ] **The clock tree diagram.** Trace one path with your finger: oscillator source → PLL → bus prescalers → a peripheral clock enable.
- [ ] **Errata sheets.** A separate document. Note what an erratum is allowed to contradict.

## Project — Answer these from the documents only

### Rules

- [ ] No search engine. No forum. No LLM. Documents only.
- [ ] For every answer: record the value, the document, and the page number.
- [ ] Where an answer comes from the schematic, note the net name or component designator too.
- [ ] If the user manual and the schematic disagree, record both and mark which one you trust.

### Setup — get the documents

- [ ] STM32F303VC datasheet.
- [ ] RM0316 reference manual.
- [ ] STM32F3DISCOVERY board user manual (UM1570).
- [ ] Board schematic, matching your board revision.
- [ ] STM32F303xB/C/D/E errata sheet.
- [ ] AN4235 (I²C timing configuration).
- [ ] Find your board revision. It is printed on the PCB. Write it down — questions 9 and 10 depend on it.
- [ ] Create the answer log file. One entry per question: answer, document, page.

### Board wiring

- [ ] **Q1.** Which GPIO port and pins are LD3–LD10, the eight-LED ring?
  - [ ] List all eight with their pin numbers.
  - [ ] Note whether they are contiguous.
- [ ] **Q2.** Which pin is the user button B1?
  - [ ] Active high or active low?
  - [ ] Pull resistor internal or external?
  - [ ] This differs from most Nucleo boards. Get it wrong and your button logic is inverted. Write down the consequence for your read logic.
- [ ] **Q10.** Does your board revision's ST-LINK expose a USB virtual COM port?
  - [ ] Which USART pins are broken out for an external adapter?
  - [ ] Note which board revision you checked, and whether the answer changes between revisions.

### Electrical limits

- [ ] **Q3.** What is the absolute maximum voltage on a non-5V-tolerant GPIO?
  - [ ] Maximum current per pin.
  - [ ] Maximum total current per port.
  - [ ] Note whether the per-port total is a sum limit you could exceed with the eight-LED ring alone.

### Registers and reset state

- [ ] **Q4.** What is the reset value of `GPIOA_MODER` on the F303?
  - [ ] Why isn't it zero? (Hint: PA13/PA14.)
  - [ ] Write down what those pins do by default and what happens if your init code clobbers them.
- [ ] **Q5.** Which register and which bit enables the clock to GPIOE?
  - [ ] Careful: on the F3, GPIO clocks are not on the same bus as on the F4. Name the bus and the register.
  - [ ] What happens if you write a GPIO register before enabling its clock? Write down the observed behaviour, not a guess.

### Clocks

- [ ] **Q6.** On reset, what clocks the CPU, and at what frequency?
  - [ ] What is the documented maximum core frequency?
  - [ ] Can you reach the maximum using the internal oscillator alone?
  - [ ] If not, why not? Write down the limiting factor from the clock tree.
  - [ ] What does your board provide as an alternative? Confirm it on the schematic, not the user manual.

### On-board peripherals

- [ ] **Q7.** Which I²C peripheral and which pins connect to the on-board e-compass?
  - [ ] What are its I²C slave addresses? It presents more than one — list them all and note what each one addresses.
- [ ] **Q8.** Which SPI peripheral, and which pin is chip-select, for the on-board gyroscope?
  - [ ] Which pins are its interrupt lines?
  - [ ] Note whether chip-select is driven by hardware or must be a plain GPIO in your code.
- [ ] **Q9.** Which MEMS parts are actually on your board? This is the one that costs people a weekend.
  - [ ] Gyroscope: L3GD20 or I3G4250D? Read the marking on the physical part, then confirm against your board revision.
  - [ ] Compass/accelerometer: LSM303DLHC or LSM303AGR?
  - [ ] `WHO_AM_I` / ID register value for each part you actually have.
  - [ ] Note that these differ per part, so a wrong assumption fails silently at the ID check. Write down what your driver should do when the ID doesn't match.

### I²C timing

- [ ] **Q11.** From AN4235 or RM0316's tables: what `TIMINGR` value gives 100 kHz standard-mode I²C?
  - [ ] State your intended I²C peripheral clock first. The answer is meaningless without it.
  - [ ] Trace on the clock tree where that peripheral clock comes from, and confirm it is what you assumed.
  - [ ] Record the table row you used, and the field breakdown of the value, not just the hex word.

### Errata pass

- [ ] Search the errata sheet for each peripheral you touched above: GPIO, RCC, I²C, SPI.
- [ ] Record any erratum that contradicts an answer you wrote. Mark that answer as superseded.
- [ ] Optional: build a one-page cheat sheet from your answer log — pin assignments, clock enable bits, and the two `WHO_AM_I` values — and keep it on the bench.

## Done when

- [ ] All eleven questions answered, each with the document name and page number.
- [ ] Questions 5, 6 and 11 — the F3-specific traps — answered from the F3 documents only, with no F4 assumptions carried over. Note explicitly where the F3 differs.
- [ ] Question 9 answered against the physical markings on your board, not against the reference design.
- [ ] You can name, out loud and without hesitating, which of the four documents answers a given question type.
- [ ] You can navigate RM0316 without panic: pick any peripheral register at random and find its offset, reset value, and access types in under two minutes.
- [ ] Keep the answer log and the errata notes. This is the reference you will work from for every peripheral week that follows.
