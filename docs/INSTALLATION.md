# Installation

## Home Assistant ESPHome Device Builder

1. Download and extract the repository.
2. In Home Assistant, open **ESPHome Device Builder**.
3. Open the existing `waveshare-home-panel` device.
4. Back up the current ESPHome folder.
5. Replace the complete YAML with `waveshare-home-panel.yaml`.
6. Copy the complete repository `sounds` folder to:
   `/config/esphome/sounds/`
7. Keep or restore any required font and image assets already used by your
   current dashboard.
8. Copy `secrets.yaml.example` to `secrets.yaml`.
9. Enter your Wi-Fi details and Home Assistant camera token.
10. Select **Validate**.
11. Select **Install**.
12. Use USB for recovery if wireless installation times out.

The startup sound plays after the loading screen reaches `READY` (only audible
once a speaker is connected).

## Flashing from Windows over USB

1. Install ESPHome in a Python 3.11+ virtual environment (`pip install esphome`).
2. Copy the project, including `secrets.yaml`, `sounds`, `weather_assets` and
   `ui_icons`, to a short **local** folder such as `C:\esp\panel`. Building
   from a network share (UNC path) fails.
3. Set a short ESP-IDF tools path, then run from PowerShell (not Git Bash):

   ```powershell
   $env:ESPHOME_ESP_IDF_PREFIX = "C:\ESPHome\idf"
   esphome run waveshare-home-panel.yaml --device COM3
   ```

   Replace `COM3` with the board's serial port.
