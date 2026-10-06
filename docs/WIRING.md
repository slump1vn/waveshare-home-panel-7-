# Wiring

## Board

The display and GT911 touch controller are integrated into the Sunton
ESP32-8048S070 and are configured in the YAML:

| Function | GPIO |
|---|---|
| Touch SDA / SCL | 19 / 20 |
| Touch reset | 38 |
| Backlight | 2 |
| Serial log (UART0 via CH340) | 43 / 44 |

Touch runs in polling mode (no interrupt pin) because the interrupt line did
not deliver touch events reliably on this board.

## MAX98357A amplifier (optional, not fitted yet)

The audio pipeline is configured, but no speaker is connected at the moment.
When you add one, wire it to the free TF-card slot pins (do not use an SD card
at the same time):

| MAX98357A | ESP32-8048S070 |
|---|---|
| BCLK | GPIO12 |
| LRC | GPIO11 |
| DIN | GPIO13 |
| VIN | 5V |
| GND | GND |
