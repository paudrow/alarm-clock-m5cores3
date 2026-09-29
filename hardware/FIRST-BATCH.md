# First batch: 10 clocks without going broke

The goal: a first run of about 10 clocks at around **$200**, where cash cost plus your time (at $40/h) comes in under the price. Ideally around **$120 per unit**.

## Honest starting point

- **The full design is ~$285–370 per unit at 10** (`BOM.md`, section 5). That uses the 4.1″ AMOLED on a $120 T-Display-P4 or a custom P4 board, singly-cut glass, and 2.5 h of shop time per clock.
- **Certification is a fixed cost**, not a per-unit one. FCC Part 15B testing is $1,000–3,000 at the cheapest labs (`CERTIFICATION.md`). Spread over 10 clocks, that alone is $100–300 each, more than the whole $120 target.
- **Two targets, two answers:**
  - **$120 all-in at 10 units isn't realistic** for an aluminium-and-glass clock with an AMOLED.
  - **~$175–185 at 10 units is**, with the lean design below. **~$110–120 is realistic at ~100 units** with the same lean design.

So the plan is to make the first 10 a **paid learning run** (pre-orders, founders pricing), and make the money on the next 50–100.

## Where the money goes, and what to cut

| Cost | Full design at 10 | Lean design at 10 | How |
| --- | --- | --- | --- |
| Brain + screen | $80–120 | **$35–45** | All-in-one ESP32-S3 + 2.41″ AMOLED touch board instead of the P4 + 4.1″ |
| Your electronics board | $40–55 | **$15–22** | Small 2-layer carrier: USB-C power, amp, lamp driver, RTC. No high-speed lines, so it's easy and cheap. |
| Glass base | $30–60 | **$10–20** | Order all 10 at once from a local glass shop. Also smaller, because the case shrinks. |
| Your time | 2.5 h ($100) | **~1.25 h ($50)** | CNC fixtures, and every step done for all 10 in one go: saw all, op 1 all, op 2 all, one anodizing trip |
| Box | $10–20 | **$5** | Plain kraft box + printed sleeve + pulp insert. No adapter; include a USB-C cable only. |

## Display options

The screen is the biggest cost, and the main thing to trade:

| Option | Touch | True black, no flicker | Digit height (reads at 10 ft?) | Brain + screen cost at 10 |
| --- | --- | --- | --- | --- |
| **A. 4.1″ AMOLED** (current plan) | Yes | Yes (measure dimming) | ~1.4″, easily | $80–120 |
| **B. 2.41″ AMOLED all-in-one** (Waveshare ESP32-S3-Touch-AMOLED-2.41, 600×450) | Yes | Yes (measure dimming) | ~0.8″ for "12:34", **~1.0″ for "9:41"** (12-hour, no leading zero) | ~$35–45 |
| **C. Big red LED digits** (1.5–1.8″, ~660 nm, each segment on its own constant-current channel with DC dimming) behind smoked glass, plus a machined aluminium **knob** | No | Yes, if driven that way (no multiplexing, no PWM) | 1.5–1.8″, easily | ~$20–30 plus a bigger custom board |

Notes on each:
- **A** is the best clock and the most expensive. It's the right choice once you're making 100+ and panel prices come down.
- **B** keeps everything you asked for except size.
  - Digits are about 20% bigger than your CoreS3's, so **you can test this tonight**: stand 10 ft from the CoreS3 and see whether ~20% bigger would do.
  - Waveshare's all-in-one boards use the same ESP32-S3 as the CoreS3, so the firmware port is small.
  - Check on the Waveshare wiki that the board exposes enough pins for I2S audio, the lamp and the RTC.
- **C** is the classic bedside-clock look, done with high-end parts.
  - Very readable, zero flicker, and cheap parts, but **no touch screen**. Settings happen on the digits with a knob, like high-end audio gear.
  - At 10 units it isn't much cheaper than B, because it needs a bigger custom board. It only wins at larger volumes.

**Recommendation:** option **B** for the first 10. It costs about the same as C, keeps touch and the AMOLED, and uses the chip you already know. Keep **A** as the premium follow-up once there's demand.

## Lean design (option B), cost per unit

| Item | At 10 | At 100 |
| --- | --- | --- |
| ESP32-S3 + 2.41″ AMOLED touch board | $35–45 | $28–35 |
| Carrier PCB, assembled (USB-C power, MAX98357A, lamp driver + 4 × 660 nm LEDs, RV-3028 + supercap) | $15–22 | $8–12 |
| Speaker (Tectonic TEBM35C10-4) | $9 | $7 |
| Enclosure materials: tube, caps, sled, glass, felt, wool, cork, screws; anodizing share | $40–50 | $22–28 |
| Box, insert, USB-C cable | $5–8 | $4–5 |
| Freight in, duties | $5 | $3 |
| Scrap allowance | $4 | $3 |
| **Cash per unit** | **$113–143** | **$75–93** |
| Your time (machining + assembly + test) | ~1.25 h = $50 | ~0.75 h = $30 |
| **Total per unit** | **~$165–195** | **~$105–125** |

The case can also **shrink**. With a 2.41″ screen, a 3″ tube about 100–110 mm long (instead of 130) still fits the speaker, and saves metal and glass.

## Selling 10 at $200 vs $249–299

These figures assume shipping is charged to the buyer, and allow ~3% for payment fees.

| Price | Margin per unit (lean, at 10) | 10 units | Covers FCC testing ($1,000–3,000)? |
| --- | --- | --- | --- |
| $200 | ~$0–30 | ~$0–300 | No |
| $249 | ~$45–75 | ~$450–750 | Partly |
| $299 | ~$95–125 | ~$950–1,250 | Mostly, at a cheap lab |

A numbered "Founders edition, 1 of 10", handmade in your shop, at **$249–299** is a fair price for what it is. Then **$199–229** for the 50–100 run, where the lean cost is ~$105–125.

## How not to go broke

1. **Set a spending cap before the first sale.** For example, $2,500 total including the prototypes. Option B prototypes are much cheaper than the P4 route: a $40 board instead of $120.
2. **Take pre-orders first.** The FCC allows advertising and taking orders before certification, as long as nothing ships until testing is done. Build and certify with customers' money, not yours. If 10 people won't pre-order, you've learned that for the cost of a web page.
3. **Order parts only for the units that are paid for,** plus 1–2 spares.
4. **Use the cheapest legitimate lab.** Supplier's Declaration of Conformity testing doesn't require an accredited lab. Do the home pre-scan first (`CERTIFICATION.md`) so you pass first time.
5. **Keep a 5% reserve** per unit for warranty returns.
6. **Batch everything:** one parts order, one PCB order, one glass order, one anodizing lot.
