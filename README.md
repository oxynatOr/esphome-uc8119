<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->
<a id="readme-top"></a>
<!--
*** Thanks for checking out the Best-README-Template. If you have a suggestion
*** that would make this better, please fork the repo and create a pull request
*** or simply open an issue with the tag "enhancement".
*** Don't forget to give the project a star!
*** Thanks again! Now go create something AMAZING! :D
-->



<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![project_license][license-shield]][license-url]
<br />
<h1 align="center">UC8119 display driver<sup>WIP</sup></h1>

## UC8119 Register Map

Reverse-engineered from I2C logic analyzer captures using Sigrok. All transactions to address 0x50 are **write-only**; no read transactions were observed in the original firmware.

| Register | Name | Bytes | Description |
|----------|------|-------|-------------|
| 0x00 | PSR | 1 | Panel Setting (always 0x0F) |
| 0x01 | PWR | 2 | Power Setting (0x46, 0x46) |
| 0x02 | POF | 1 | Power OFF (0x03) |
| 0x03 | PFS | 1 | Mode select: 0x00=Clear, 0x06=Normal |
| 0x04 | PON | 0 | Power ON (command only, no data) |
| 0x12 | DRF | 0 | Display Refresh trigger |
| 0x15 | VCOM | 3 | VCOM timing (0x00, 0x87, 0x00) |
| 0x18 | FB | 17+1 | Framebuffer (17 data bytes + 0x80 terminator) |
| 0x1C | OLDFB | 17+1 | Previous framebuffer (for diff updates) |
| 0x20 | LUTC | 15 | LUT VCOM |
| 0x23 | LUTWB | 15 | LUT White-to-Black |
| 0x24 | LUTBB | 15 | LUT Black-to-Black |
| 0x25 | LUT3 | 15 | LUT Extra |
| 0x26 | LUTBD | 15 | LUT Border |
| 0x30 | PLL | 1 | PLL Control (0x07) |

### Framebuffer Format

The framebuffer consists of 17 data bytes (136 bits) followed by a 0x80 terminator byte. Segment logic is **inverted**: 0=segment ON (black), 1=segment OFF (white).<br>

Bit numbering: <br>
Byte 0 Bit 7 = Segment 0 <br>
Byte 0 Bit 6 = Segment 1 <br>
etc. Of the 136 possible segments, 91 are connected to visible display elements; the remaining 45 bits are unused.

### BUSY_N Pin Timing

Measured from logic analyzer captures with correlation to GPIO6 (channel D7):

| Trigger | Duration | Notes |
|---------|----------|-------|
| DRF (Clear mode, PFS=0x00) | ~1870ms | Used during ghost clear |
| DRF (Normal mode, PFS=0x06) | ~874ms | Standard refresh |
| PON (0x04) | 17-68ms | Power-up delay |
| POF (0x02) | ~10ms | Power-down |

The original firmware polls BUSY (not timer-based delays), confirmed by I2C transaction gaps correlating exactly with BUSY LOW periods.

## Refresh Sequences

Four distinct refresh sequences were identified in the original firmware:

### Type A: Boot/Clear Sequence (~1870ms)

Used during initial power-up. Clears the display to a known state.

```
PON → wait_busy → PSR(0x0F) → PWR(0x46,0x46) → PLL(0x07)
→ PFS(0x00)                    ← Clear mode
→ LUTC(clear) → LUTWB(clear) → LUTBD(clear)   ← 3 LUTs only
→ FB(0x00..0x00, 0x80)        ← All segments ON
→ DRF → wait_busy(~1870ms)
→ POF(0x03) → wait_busy
```

### Type B: Full Refresh (~874ms × 2)

Two-phase refresh: white flash followed by content. Reduces ghosting.

```
Phase 1 (white flash):
  PON → config → PFS(0x06) → VCOM(0x00,0x87,0x00)
  → 5 LUTs (LUTC, LUTWB, LUTBB, LUT3, LUTBD)
  → FB(0xFF..0xFF, 0x80)      ← All segments OFF
  → DRF → wait_busy(~874ms)

Phase 2 (content):
  → FB(content, 0x80)
  → DRF → wait_busy(~874ms)
  → POF(0x03)
```

### Type C: Partial Update (~874ms)

Standard content update without white flash. Used for normal operation.

```
PON → config → PFS(0x06) → VCOM → 5 LUTs
→ FB(content, 0x80)
→ DRF → wait_busy(~874ms)
→ POF(0x03)
```

### Type D: Wake + Restore

After hardware reset, writes both 0x1C (old FB) and 0x18 (new FB) for differential update. Allows the display to transition from its last known state to new content without a full refresh.

## LUT Tables

All LUT tables are 15 bytes each. Captured from the original firmware:

### Clear Mode (3 LUTs)

```
LUTC  (VCOM):      5E BC 01 9E 7C 01 00 00 00 00 00 00 00 00 00
LUTWB (White→Blk): 9E BC 01 5E 7C 01 00 00 00 00 00 00 00 00 00
LUTBD (Border):    9E 7C 01 5E BC 01 00 00 00 00 00 00 00 00 00
```

### Normal Mode (5 LUTs)

```
LUTC  (VCOM):      68 A8 01 81 00 01 41 00 01 00 00 00 00 00 00
LUTWB (White→Blk): 68 A8 01 41 00 01 41 00 01 00 00 00 00 00 00
LUTBB (Blk→Blk):  A8 A8 01 81 00 01 41 00 01 00 00 00 00 00 00
LUT3  (Extra):     68 68 01 81 00 01 41 00 01 00 00 00 00 00 00
LUTBD (Border):    68 A8 01 81 00 01 81 00 01 00 00 00 00 00 00
```

## UltraChip Family Comparison

Compared with UC8151c, UC8171c, UC8175c, UC8176c datasheets from public sources.

8 of 14 identified registers match the UC81xx EPD family exactly (PSR, PWR, PON, DRF, LUTs, PLL). UC8119-specific registers include 0x15 (VCOM timing), 0x18 (segment framebuffer instead of 0x10 DTM used in dot-matrix controllers), and 0x1C (old framebuffer).

Closest relative: **UC8175c** (80×160, B/W, designed for cards and small displays). The UC16xx series (UC1611s, UC1617w) does NOT match — completely different architecture, though the I2C address 0x50 matches the UC16xx default addressing scheme (A1=A0=GND).

## Complete Segment Mapping

Mapped by bit-scanning: setting one bit at a time in the framebuffer and photographing the display.

### Digit Positions (7-Segment: A=top, B=top-right, C=bot-right, D=bottom, E=bot-left, F=top-left, G=middle)

| Digit | Function | A | B | C | D | E | F | G |
|-------|----------|---|---|---|---|---|---|---|
| T1 | Clock tens hour | 12 | 26 | 32 | 28 | 19 | 17 | 29 |
| T2 | Clock ones hour | 13 | 23 | 35 | 37 | 36 | 24 | 34 |
| T3 | Clock tens min | 14 | 18 | 44 | 131 | 40 | 22 | 39 |
| T4 | Clock ones min | 8 | 7 | 9 | 11 | 30 | 15 | 10 |
| D1 | Temp tens | 41 | 129 | 128 | 43 | 21 | 20 | 42 |
| D2 | Temp ones | 95 | 91 | 92 | 94 | 96 | 127 | 93 |
| D3 | Temp decimal | 89 | 85 | 51 | 87 | 88 | 90 | 86 |
| H1 | Humi tens | 55 | 58 | 61 | 62 | 60 | 57 | 59 |
| H2 | Humi ones | 74 | 76 | 75 | 72 | 71 | 56 | 73 |
| UNIT | Temp unit (C/F) | 53 | 79 | 84 | 81 | 77 | 54 | 78 |

**Note:** D2-F (Bit 127) is in Byte 15, far from other D2 segments (Bytes 11-12). T3-D (Bit 131) and D1-B (Bit 129) are also in the upper byte range.

### Icons and Special Segments

| Segment | Bit | Description |
|---------|-----|-------------|
| BATT_5 | 0 | Battery frame (always on when battery shown) |
| BATT_4 | 1 | Battery bar 4 |
| BATT_3 | 2 | Battery bar 3 |
| BATT_2 | 3 | Battery bar 2 |
| BATT_1 | 4 | Battery bar 1 (lowest) |
| PFEIL | 5 | Arrow/triangle icon |
| SIG_1 | 6 | Signal bar 1 (smallest) |
| SIG_2 | 16 | Signal bar 2 |
| SIG_3 | 25 | Signal bar 3 |
| SIG_4 | 33 | Signal bar 4 (largest) |
| FROST | 27 | Snowflake icon |
| COLON | 38 | Clock colon (both dots) |
| BT | 45 | Bluetooth icon |
| GLOBE | 46 | Globe / WiFi icon |
| HEIZ | 47 | Heating (waves) icon |
| VENT | 48 | Ventilator / fan icon |
| KALEN | 49 | Calendar icon |
| DP | 50 | Decimal point (between D2 and D3) |
| GRAD | 52 | Degree symbol (°) |
| PROZENT | 83 | Percent symbol (%) |
| COL_MID | 130 | Middle colon dot (unused — creates unwanted 3rd dot) |

### Unused Bits

45 bits are not connected to visible segments, mostly in Bytes 8 and 12-15. These are reserved/unused COM/SEG lines in the UC8119 that have no corresponding LCD segment on this panel.

## I2C Analysis Methodology

### Tools Used

- **Sigrok + PulseView**: Logic analyzer capture and I2C decoding
- **esp_dump.py**: Custom 4× flash dump with SHA256 verification
- **esp_bootlog.py**: Timestamped serial boot log recorder

### Capture Setup

```
Logic Analyzer Channels:
  D1 = SCL (GPIO3)
  D3 = SDA (GPIO1)
  D7 = BUSY_N (GPIO6)
  D2 = RESET_N (GPIO7)
  D4 = ENABLE_N (GPI10)

Decode: sigrok-cli -P i2c:scl=D1:sda=D3
```

### Key Finding: BUSY Pin Behavior

The BUSY pin goes LOW immediately after DRF (Display Refresh) command and returns HIGH when the refresh is complete. 
During RESET and deep sleep, the BUSY pin becomes high-impedance.



<!-- LICENSE -->
## License



<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[wip-shield]: https://img.shields.io/github/contributors/oxynatOr/esphome-uc8119.svg?style=for-the-badge
[contributors-shield]: https://img.shields.io/github/contributors/oxynatOr/esphome-uc8119.svg?style=for-the-badge
[contributors-url]: https://github.com/oxynatOr/esphome-uc8119/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/oxynatOr/esphome-uc8119.svg?style=for-the-badge
[forks-url]: https://github.com/oxynatOr/esphome-uc8119/network/members
[stars-shield]: https://img.shields.io/github/stars/oxynatOr/esphome-uc8119.svg?style=for-the-badge
[stars-url]: https://github.com/oxynatOr/esphome-uc8119/stargazers
[issues-shield]: https://img.shields.io/github/issues/oxynatOr/esphome-uc8119.svg?style=for-the-badge
[issues-url]: https://github.com/oxynatOr/esphome-uc8119/issues
[license-shield]: https://img.shields.io/github/license/oxynatOr/esphome-uc8119.svg?style=for-the-badge
[license-url]: https://github.com/oxynatOr/esphome-uc8119/blob/main/LICENSE
[Next.js]: https://img.shields.io/badge/next.js-000000?style=for-the-badge&logo=nextdotjs&logoColor=white
[Next-url]: https://nextjs.org/
[React.js]: https://img.shields.io/badge/React-20232A?style=for-the-badge&logo=react&logoColor=61DAFB
[React-url]: https://reactjs.org/
[Vue.js]: https://img.shields.io/badge/Vue.js-35495E?style=for-the-badge&logo=vuedotjs&logoColor=4FC08D
[Vue-url]: https://vuejs.org/
[Angular.io]: https://img.shields.io/badge/Angular-DD0031?style=for-the-badge&logo=angular&logoColor=white
[Angular-url]: https://angular.io/
[Svelte.dev]: https://img.shields.io/badge/Svelte-4A4A55?style=for-the-badge&logo=svelte&logoColor=FF3E00
[Svelte-url]: https://svelte.dev/
[Laravel.com]: https://img.shields.io/badge/Laravel-FF2D20?style=for-the-badge&logo=laravel&logoColor=white
[Laravel-url]: https://laravel.com
[Bootstrap.com]: https://img.shields.io/badge/Bootstrap-563D7C?style=for-the-badge&logo=bootstrap&logoColor=white
[Bootstrap-url]: https://getbootstrap.com
[JQuery.com]: https://img.shields.io/badge/jQuery-0769AD?style=for-the-badge&logo=jquery&logoColor=white
[JQuery-url]: https://jquery.com 
