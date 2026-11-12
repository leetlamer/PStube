# PStube

Standalone native YouTube homebrew for PlayStation 5, with a blue SmartTube-inspired interface.
Current milestone: **PS5 beta 0.16**. Owner-tested on PS5 Pro firmware 13.00 using
OnionHEN 0.9.0 / websrv 0.34. This repository snapshot contains the PS5 app.

Features include category shelves, search, accounts and libraries, software video
playback, seeking, captions, SponsorBlock, optional DeArrow and app-level DNS/proxy
settings. Native 4K and live playback remain experimental; no hardware decoding is claimed.

- [Beta guide](pstube-ps5/docs/BETA_V016.md)
- [Test results and limitations](pstube-ps5/docs/BETA_V016_TEST_RESULTS.md)
- [Feature status](pstube-ps5/docs/BETA_FEATURE_MATRIX.md)
- [App and build notes](pstube-ps5/README.md)

## Source snapshot

Keep the `pstube-ps5/` directory layout: build scripts resolve SDKs and dependencies
relative to its parent. SDKs, downloaded archives, local toolchains, generated builds,
console logs, accounts and private configuration are excluded. Dependency versions
and hashes are recorded under `pstube-ps5/third_party/`. Setup currently expects the
existing workspace dependency layout; a fresh-clone bootstrap is still needed.
Host decoder tests currently reference the earlier PS4 workspace's host libraries,
which are not included in this PS5 snapshot. Test videos are generated separately
using the supplied fixture scripts and are not committed.

PStube is GPLv3-or-later; imported assets and dependencies retain their own notices.
See [third-party notices](THIRD_PARTY.md) and the asset provenance manifests.
PStube is an independent project and is not affiliated with Sony, Google or SmartTube.
