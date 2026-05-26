# Dice_rce

Prototyping the encoder by printing the direction in the serial monitor.
The reusable library lives in `lib/Encoder_lib/` and can be dropped into
any PlatformIO project.

NOT PERFECT but works.

## Quick overview

1. Wire the encoder: pin A and pin B to two GPIOs, the third pin to GND.
   No external resistors needed (internal pull-ups are used).

2. Copy the `Encoder_lib/` folder into your project's `lib/` folder.

3. See the Doc in the Encoder.h



