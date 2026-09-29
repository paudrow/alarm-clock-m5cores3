# Certification and compliance

What you need before selling the clock, by market, and what it costs. Costs are rough; get quotes from two or three labs.

**Not legal advice.** Rules change. Before launch, have a test lab or compliance consultant confirm the list for the exact markets you'll ship to.

## Why this product is easy to certify

Three decisions already made keep this cheap:

| Decision | What it avoids |
| --- | --- |
| **No radio** (no Wi-Fi/Bluetooth) | No FCC ID, no radio testing, no EU Radio Equipment Directive. The product is only an "unintentional radiator": a digital device that must not emit too much noise. |
| **5 V over USB-C, with a bought-in certified adapter** | Nothing touches mains. Mains safety belongs to the adapter's maker. |
| **Supercap instead of a coin cell or LiPo** | No battery testing (UN38.3, IEC 62133), no battery shipping rules. In the US, no **Reese's Law** (16 CFR 1263): products containing button or coin cells need child-resistant battery compartments, warnings and testing. |

Adding Wi-Fi or a battery in V4 changes this. Use a pre-certified radio module and a pre-tested battery pack to inherit their approvals.

## United States (launch here first)

| Requirement | What it means for you | ≈ Cost |
| --- | --- | --- |
| **FCC Part 15 Subpart B, Class B** (residential digital device) | A lab measures radiated and conducted emissions (conducted is measured through the USB-C adapter). You then sign a **Supplier's Declaration of Conformity (SDoC)**. No filing with the FCC. You need a **US responsible party** (you) and must keep the test report. | $1,500–3,000 |
| FCC labelling | A compliance statement in the manual or on the packaging. The FCC logo is optional. | — |
| **Power adapter**, if you ship one | Buy one that is already UL/ETL listed and meets DOE Level VI efficiency. | in the BOM |
| Product safety | No federal certification is required for a 5 V product. **Amazon and big retailers often ask for an IEC 62368-1 test report**, and it also helps with insurance. Optional at first. | $3,000–8,000 if you get it |
| California **Prop 65** | Only if the product contains listed chemicals above thresholds. Watch for **leaded brass** (brass standoffs or button caps); stick to aluminium and stainless and it likely doesn't apply. | — |
| "Made in USA" claims | The FTC standard is "all or virtually all" US content. With a Chinese panel and chips, use **"Designed and assembled in [your state]"** or "Assembled in USA" (the case is made and the product assembled by you). Don't use an unqualified "Made in USA". | — |
| Written warranty | If you offer one, the Magnuson-Moss Act sets rules for how it's written. Keep it simple and clear. | — |
| State e-waste laws | Some are written around screens larger than 4″ diagonal, and the 4.1″ panel is just over. They mostly target TVs, monitors, laptops and tablets, and a clock is probably not covered. Ask whoever reviews your compliance. | — |

**Canada** is nearly free to add: **ICES-003 Class B** is essentially the same emissions test. Most labs run both together. Add the label text `CAN ICES-003(B)/NMB-003(B)`.

## European Union (CE marking), later

| Requirement | What it means | ≈ Cost |
| --- | --- | --- |
| **EMC Directive**: EN 55032 (emissions) + **EN 55035 (immunity)** | Emissions as for the FCC, plus immunity: **ESD** (the metal case gets zapped, so the case-to-ground bond matters), radiated RF, fast transients and surges through the adapter. | $3,000–5,000 |
| Low Voltage Directive | **Doesn't apply**: it starts at 50 V AC / 75 V DC. | — |
| **General Product Safety Regulation** (GPSR, since Dec 2024) | Risk assessment, traceability (batch or serial on the product), and **an economic operator based in the EU**. An importer, distributor or paid "EU responsible person" service can fill that role. An EN IEC 62368-1 report is the easy way to show safety. | $300–1,500/yr for a responsible-person service |
| **RoHS** | Collect RoHS declarations for every part (distributors provide them) and keep them in the technical file. | — |
| **REACH** | Declare substances of very high concern above 0.1% by weight. Lead in brass is the usual one, so avoid brass. | — |
| **WEEE** | Register as a producer in **each** EU country you sell into, add the crossed-out-bin symbol, and pay take-back fees. | ~$200–1,000/yr per country, or use a compliance service |
| Packaging EPR | Register packaging in countries that require it (Germany's LUCID, French Triman labelling). | small |
| Ecodesign standby rules (EU 2023/826) | Check whether the clock's "off" or low-power states fall under them. The clock is normally always on, so they may not apply. | — |
| **Declaration of Conformity + technical file** | You write the DoC, keep schematics, BOM, test reports and manual for 10 years, and put the CE mark and your address on the product. | — |

**UK:** CE marking is currently accepted for these product types, alongside UKCA. A UK-based responsible person is needed, and there's UK WEEE registration.

## What the lab tests, and how to pass first time

- **Radiated emissions** are the usual failure. The likely sources on this design:
  - the MIPI-DSI lines to the screen;
  - the ESP32-P4's clocks and core buck converter;
  - the class-D amp's speaker leads.

  Keep the DSI routing tight over a solid ground plane, keep the speaker wires short and twisted with ferrites, and bond the aluminium case to ground. A grounded metal box is an excellent shield.
- **Conducted emissions** are measured on the mains side of the USB-C adapter. Good input filtering on VBUS (a ferrite bead and capacitors) keeps your switching noise off the cable.
- **ESD (EU)**: ±8 kV air and ±4 kV contact on the case, screen, button and USB-C. The case bond, the USB ESD array and the TVS diode are there for this.

**Pre-compliance at home** is cheap and saves a failed lab day:
- a TinySA or similar small spectrum analyser plus a set of near-field probes (~$100–200) finds the noisy spots on the board;
- then book 2–4 hours of pre-scan at a lab ($500–1,500) before the formal test.

## Paperwork to keep (all markets)

- Test reports (FCC/ICES, EN 55032/55035, 62368-1 if done)
- Declarations: FCC SDoC, EU DoC
- Schematics, BOM, and RoHS declarations for the parts
- User manual with compliance statements and safety notes
- Label artwork: model, serial/batch, maker name and address, CE/FCC/ICES text, WEEE bin

Also not certification, but you'll want **product liability insurance** before selling. Small hardware businesses typically pay ~$500–2,000 per year.

## Selling small batches

- **There's no small-batch exemption.** Selling even 10 clocks means the FCC testing is done first.
- **You can advertise and take pre-orders before authorization.** The FCC allows marketing and conditional sales as long as **nothing is delivered until testing is complete**. Say so clearly on the order page.
- **Loaning prototypes** to friends to evaluate (not selling them) is the usual way to get feedback before then.

## Suggested order

1. **US + Canada first:** FCC Part 15B and ICES-003 in one lab session, about **$2,000–4,000** including a pre-scan.
2. Add the IEC 62368-1 safety report when a retailer or insurer asks for it.
3. **EU/UK later:** EN 55032/55035, an EU responsible person, WEEE and packaging registration. Budget about **$5,000–10,000** up front plus a few hundred a year per country.
