# Changelog

## v121 — Sunton ESP32-8048S070 port

- Switched the display to the `ESP32-8048S070` model and removed the CH422G expander.
- Moved touch (GT911), backlight and I2C pins to the Sunton board; touch uses polling.
- Logging now uses UART0; audio pins moved to GPIO11/12/13 (speaker not fitted yet).
- Removed `execute_from_psram`, enlarged the PSRAM cache line, and set the pixel
  clock to 10 MHz (inverted) to stop screen flicker.
- Pointed Home Assistant to `https://180.elmarspices.com:8123` and switched to
  Frigate camera entities.
- Added the missing weather and UI icon images.

## v120 — Modern startup audio

- Replaced the startup sound with the supplied modern smart-home WAV.
- Optimised startup audio for the 16 kHz MAX98357A announcement pipeline.
- Preserved the complete Version 119 dashboard configuration.
- Added GitHub-ready documentation and repository files.

## v119 — Bin collection

- Added Home Assistant bin collection day and bin type information.
- Added automatic colour coding for collection timing and bin type.
