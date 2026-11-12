<p align="center">
  <img src="pstube-ps5/branding/GlossyBluePStubePlayButton.png" alt="PStube logo" width="200">
</p>

# PStube

**An ad-free YouTube experience for PlayStation 5.**

PStube is a standalone native homebrew app with a blue TV interface, controller
navigation, Google account support and software-decoded video playback.
Current release: **PS5 beta 0.16**.

## Requirements

- A PS5 configured to run compatible homebrew, with **OnionHEN active**.
- **websrv (Homebrew Launcher)** enabled to launch the app in a foreground graphics context.
- An internet connection for YouTube. Google login is optional.
- USB or FTP access to copy the app folder onto the console.

The tested configuration is **PS5 Pro firmware 13.00, OnionHEN 0.9.0 and websrv 0.34**.
These are verified test versions, not a claim that every firmware or launcher version
is supported. Installing PStube does not install OnionHEN or enable homebrew by itself.
Playback runs on the console; no PC transcoding server is required.

## Download and install

1. Download the install ZIP from [Releases](https://github.com/leetlamer/PStube/releases).
2. Extract it and copy the complete `PStube-PS5-Test` folder into `/data/homebrew/`.
3. Enable OnionHEN and websrv, open the Homebrew Launcher and launch `eboot.elf` in the foreground.
4. Preserve existing account and preference files in `/data` when updating.

## Features

- Home, Music, Gaming, News and Live category shelves, search and Shorts.
- Larger thumbnails, scrolling titles/descriptions and controller navigation.
- Google account linking, saved accounts, subscriptions, history and playlists.
- Playback controls, seeking, captions, video metrics and automatic next-video options.
- Optional SponsorBlock to skip creator sponsorship segments and optional DeArrow.
- App-level DNS-over-HTTPS and proxy settings.

**1080p is the recommended default.** Native 4K and live playback remain experimental
and source-dependent. Hardware decoding and HDMI refresh switching are not implemented.
DeArrow alternate titles are verified; replacement thumbnails need further testing.

[Beta guide](pstube-ps5/docs/BETA_V016.md) ·
[Test results](pstube-ps5/docs/BETA_V016_TEST_RESULTS.md) ·
[Feature status](pstube-ps5/docs/BETA_FEATURE_MATRIX.md)

## Building from source

Keep the `pstube-ps5/` layout: scripts resolve dependencies relative to its parent.
SDKs, downloaded dependencies, toolchains, generated builds, console logs and private
configuration are excluded. Versions/hashes are under `pstube-ps5/third_party/`.
Setup currently expects the existing development dependency layout; a fresh-clone
bootstrap is still pending. Host decoder tests depend on earlier workspace host
libraries. Fixture scripts regenerate test media.

PStube is GPLv3-or-later. Imported assets and dependencies retain their required
licences and attribution; see [third-party notices](THIRD_PARTY.md).
PStube is independent and is not affiliated with Sony or Google.
