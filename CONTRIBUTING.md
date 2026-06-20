# Contributing to DYCE

Thanks for taking a look 🙂 — DYCE is a learning-in-the-open hardware project, so
**all feedback and PRs are welcome**: schematic nitpicks, firmware cleanups,
enclosure ideas, docs, typos, anything.

## Repo layout

```
Firmware/   ESP32-S3 firmware (PlatformIO) — main.cpp, dice.*, ui.*, rng.*, config.h
Dyce/       web simulator (the live demo)
Media/      renders, photos, Gallery.md
PCB/        schematic (PDF)
```

## Build the firmware

```bash
cd Firmware
pio run            # build
pio run -t upload  # flash an ESP32-S3
pio device monitor # serial @115200
```

The firmware is **100% offline** — no Wi-Fi, no network, no accounts. Randomness
comes from the ESP32 hardware RNG.

## How to contribute

1. **Open an issue** first for anything non-trivial, so we can agree on the approach.
2. Fork & branch: `feature/short-name` or `fix/short-name`.
3. Keep changes focused; match the existing style (small functions, clear names,
   one concern per file — `dice` = logic, `ui` = drawing, `rng` = randomness).
4. If it touches the firmware, make sure `pio run` still builds.
5. Open a PR describing **what** changed and **why**.

## Ground rules

- Be kind and constructive — this is a student's first real hardware project.
- No secrets in commits (Wi-Fi creds, tokens, keys). There's a `.gitignore` for that.
- Photos/renders in `Media/` belong to the author; please don't re-edit them.

Not sure where to start? Browse the [issues](https://github.com/merlin-rce/Dyce/issues)
or just say hi.
