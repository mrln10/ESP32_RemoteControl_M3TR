# RotaryEncoder Modul

## Pins
Konfiguration in `include/config.h`:
- ENC_CLK
- ENC_DT
- ENC_SW

Encoder-Pins:
- CLK = Channel A
- DT  = Channel B
- SW  = Button (gegen GND)
- +   = 3V3
- GND = GND

## Verwendung
1. In `setup()`:
```cpp
initRotaryEncoder();
