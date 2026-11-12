# PStube

Standalone native YouTube homebrew for PlayStation 5 with a blue SmartTube-inspired interface.
Current milestone: **PS5 beta 0.16**, owner-tested on PS5 Pro firmware 13.00 using OnionHEN 0.9.0 / websrv 0.34.

## Source download

This initial repository import stores the source as an archive: [Download PStube-source-v0.16.zip](PStube-source-v0.16.zip).

SHA256: `f146fc02edd3651b21efebf53b1818f36607cf7b25cb70ea6fa593dada5d7114`.

Extract it to browse the 135 source, test, asset and documentation files, including the normal `pstube-ps5/` tree. Individual files are not yet expanded into Git history because command-line GitHub access was unavailable during this import. No installable console binary is included in this source snapshot.

Features include category shelves, search, accounts and libraries, software video playback, seeking, captions, SponsorBlock, optional DeArrow and app-level DNS/proxy settings. Native 4K and live playback remain experimental. No hardware decoding is claimed.

After extraction, see `pstube-ps5/docs/BETA_V016.md`, `pstube-ps5/docs/BETA_V016_TEST_RESULTS.md` and `pstube-ps5/docs/BETA_FEATURE_MATRIX.md`.

## Build status

Keep the `pstube-ps5/` directory layout: scripts resolve dependencies relative to its parent. SDKs, downloaded archives, local toolchains, generated builds, test videos, console logs, accounts and private configuration are excluded. Versions and hashes are recorded under `pstube-ps5/third_party/`. Setup currently expects the existing workspace dependency layout; a fresh-clone bootstrap remains to be added. Host decoder tests reference the previous PS4 workspace host libraries, which are not part of this PS5 snapshot. Supplied fixture scripts regenerate test videos.

PStube is GPLv3-or-later; imported assets and dependencies retain their own notices. See [third-party notices](THIRD_PARTY.md), [LICENSE](LICENSE) and asset provenance in the source archive. PStube is independent of Sony, Google and SmartTube.
