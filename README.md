<p align="center">
  <img src="pstube-ps5/branding/GlossyBluePStubePlayButton.png" alt="PStube" width="180">
</p>

# PStube

**An ad-free YouTube experience for PlayStation 5, no paid subscriptions required.**

PStube is a standalone native homebrew app, controller navigation,
Google account support and software-decoded playback. Playback runs on the console;
no PC transcoding server is required.

**Beta 0.20** · [Downloads](https://github.com/leetlamer/PStube/releases) ·
[Beta guide](pstube-ps5/docs/BETA_V020.md) ·
[Test results](pstube-ps5/docs/BETA_V020_TEST_RESULTS.md) ·
[Feature status](pstube-ps5/docs/BETA_FEATURE_MATRIX.md)

## Requirements

- A PS5 with a compatible homebrew environment and **OnionHEN active**. (found in Payload Manager)
- **websrv (Homebrew Launcher)** to launch PStube in a foreground graphics context. (Payload manager)
- An internet connection. nano DNS or similar recommended. Google sign-in is optional.
- USB or FTP access to copy the application folder onto the console.

Tested on **PS5 Pro firmware 13.00, OnionHEN 0.9.0 and websrv 0.34**.
These are verified test versions, not a compatibility guarantee for other configurations.
Installing PStube does not install OnionHEN or enable homebrew.

## Installation

1. Download `PStube-PS5-Test-v0.20.zip` from [Releases](https://github.com/leetlamer/PStube/releases).
2. Exit an existing PStube session before updating.
3. Extract the ZIP and copy the complete `PStube-PS5-Test` folder into `/data/homebrew/`.
4. Enable OnionHEN and websrv, then launch `eboot.elf` through the Homebrew Launcher.

Keep existing account and preference files in `/data` when updating. Release assets include
checksums; the source and relinking archive is intended for developers, not installation.

## Features

- Full YouTube UI navigation.
- Four-column result cards, populated thumbnails, animated scrolling and a sliding sidebar.
- Google device-code/QR sign-in, saved accounts, subscriptions, playlists and history.
- Streaming with bounded reconnection and four saved buffer presets under **Settings → Playback**.
- Pause, storyboard seek previews, accelerated L2/R2 seeking, previous/next, automatic next/loop/stop, captions and playback information.
- Automatically populated channel, views, date, likes and subscriber metadata; unavailable counts remain explicit.
- Flat four-column Subscriptions, History and Liked Videos, with dedicated portrait cards for Shorts.
- Centered animated loading indicator and controller input during video network waits.
- SponsorBlock skips and DeArrow, plus app-level DNS and proxy settings.

## Playback limits

**1080p is the recommended default.** Native 4K and live playback remain experimental and
source-dependent. The app uses software decoding; hardware acceleration, HDR output and
HDMI refresh switching are not implemented. Buffer targets are approximate and memory-bounded.
Larger buffers cannot resolve sustained decoder overload or a stream rejected with HTTP403.
YouTube does not return all counts for every video; missing dislikes are not estimated.

2K and 4k Native hardware decoding is next development milestone.

## Screenshots

Screenshots supplied from the current PS5 testing session.

**Home**

![Home](pstube-ps5/screenshots/Main%20Navigation%20Home.jpg)

**Gaming**

![Gaming](pstube-ps5/screenshots/Main%20Navigation%20Gaming.jpg)

**Live**

![Live](pstube-ps5/screenshots/Main%20Navigation%20Live.jpg)

**Shorts**

![Shorts](pstube-ps5/screenshots/Main%20Navigation%20Shorts.jpg)

**Video cards**

![Video cards](pstube-ps5/screenshots/Thumbnail%20Card%20Display.jpg)

**Playback controls**

![Playback controls](pstube-ps5/screenshots/Playback%20Example%201.jpg)

**Playback and seeking**

![Playback and seeking](pstube-ps5/screenshots/Playback%20example%202.jpg)

## Development

The application source is checked into `pstube-ps5/src/`. Installation packages and relinking
archives belong in Releases; a source ZIP is not needed to browse or contribute to the code.

| Path | Contents |
|---|---|
| `pstube-ps5/src/` | Native application, player, transport and UI |
| `pstube-ps5/assets/`, `branding/`, `sce_sys/` | Runtime assets and application artwork |
| `pstube-ps5/scripts/` | SDK setup, build, test and packaging tools |
| `pstube-ps5/tests/` | Host checks and UI previews |
| `pstube-ps5/docs/` | Beta guide, verification and feature status |
| `pstube-ps5/third_party/` | Dependency notices and provenance |

See [Building](docs/BUILDING.md) and [Contributing](CONTRIBUTING.md).
Fresh-clone console builds still require the documented dependency layout; repository CI
checks source integrity and portable logic, not firmware compatibility or console performance.

## License

PStube is GPL-3.0-or-later. Imported components retain their required copyright and license
notices; see [LICENSE](LICENSE) and [third-party notices](THIRD_PARTY.md).
PStube is independent and is not affiliated with Sony or Google.
