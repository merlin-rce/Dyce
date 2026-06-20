# DYCE — web simulator

A faithful browser simulator of the DYCE electronic dice, to show the project
without the hardware. It's a 1:1 port of the firmware: the same state machine
(`dice.cpp`) and the same rendering (`ui.cpp`), drawn on a `<canvas>`.

**Live:** intended for `merlin-rce.tech/Dyce`

## Controls
- **Scroll** on the dice = turn the ring encoder
  - scroll **up** → charge the ring & roll
  - scroll **down** → change the difficulty category (… 99 → Contact QR)
  - pause around **2/3** of the ring → unlock the hidden secret (67)
- Click **Power** = the device general switch (on / off)
- (bonus) arrow keys also work

## Run locally
It's a static site — just open `index.html`, or serve the folder:
```bash
python -m http.server      # then visit http://localhost:8000
```

## Deploy
Static hosting. Copy this folder to your web root under `/DYCE`. It needs
internet only for the Montserrat web font (everything else is local).

## Files
| File | Role |
|---|---|
| `index.html` | the page (device mockup + power switch) |
| `style.css` | minimalist styling |
| `app.js` | the simulator (ported game logic + canvas renderer) |
| `assets/logo.png` | the logo (dark theme) |
| `assets/qr.png` | the Contact QR code |
