# Wiring

## MAX98357A amplifier

| MAX98357A | Waveshare panel |
|---|---|
| BCLK | GPIO43 / UART2 TXD |
| LRC | GPIO44 / UART2 RXD |
| DIN | GPIO6 / Sensor AD |
| VIN | 5V |
| GND | GND |

The display and touch controller are integrated into the Waveshare
ESP32-S3 Touch LCD 7-inch panel and are configured in the YAML.
