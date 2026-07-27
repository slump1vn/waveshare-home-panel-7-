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

The startup sound plays after the loading screen reaches `READY`.
