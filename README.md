# ESP32-S3 Home Assistant Panel

A full-screen ESPHome and LVGL dashboard for the Sunton **ESP32-8048S070**
(ESP32-S3, 7-inch 800 × 480 RGB touch display).

> The project started on the Waveshare ESP32-S3 Touch LCD 7-inch and has been
> ported to the Sunton ESP32-8048S070. The YAML now targets the Sunton board.

## Features

- 800 × 480 touch dashboard
- Home Assistant lighting and switch controls
- Climate controls
- Met Office weather and five-day forecast
- Home energy display and high-power alerts
- Camera proxy images
- Alarm, person, parcel and doorbell notifications
- Media player and sounds using an optional MAX98357A I2S amplifier
- Modern embedded startup sound
- Clock screensaver
- Bin collection information

## Screenshot

![Home Assistant Control Centre](docs/images/dashboard-screenshot.png)

## Licence and usage

This project is shared for personal, educational and hobby use.

**Commercial use is not permitted without the author's permission.**

## Hardware

- Sunton ESP32-8048S070 (ESP32-S3, 16 MB flash, 8 MB octal PSRAM, GT911 touch)
- Optional: MAX98357A I2S amplifier and a suitable speaker (no speaker is fitted
  yet; audio is configured but unused)
- Home Assistant with ESPHome Device Builder

## Repository contents

```text
waveshare-home-panel.yaml
sounds/
weather_assets/
ui_icons/
docs/
secrets.yaml.example
CHANGELOG.md
LICENSE
```

## Important before compiling

This repository includes the WAV files, weather images and UI icons referenced
by the dashboard. Keep the `sounds`, `weather_assets` and `ui_icons` folders
beside `waveshare-home-panel.yaml` when compiling.

Never commit your real `secrets.yaml` or Home Assistant access token.

## Installation

See [docs/INSTALLATION.md](docs/INSTALLATION.md).

## Uploading to GitHub

See [docs/GITHUB_UPLOAD.md](docs/GITHUB_UPLOAD.md).

## Wiring

See [docs/WIRING.md](docs/WIRING.md).

## Version

Current project version: **v121 — Sunton ESP32-8048S070 port**

## Icon credits

- `weather_assets/` – [Meteocons](https://github.com/basmilius/weather-icons) by Bas Milius (MIT), rendered to PNG (160px main, 48px forecast).
- `ui_icons/` – [Material Design Icons](https://pictogrammers.com/library/mdi/) (Apache-2.0), rendered to white 32px PNG.
