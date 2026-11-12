# PStube PS5

An ad-free YouTube experience for PS5. Requires active OnionHEN and websrv
(Homebrew Launcher); tested with OnionHEN 0.9.0 and websrv 0.34.

Native PStube beta 0.17 for the owner-tested PS5 Pro firmware 13.00 environment,
OnionHEN 0.9.0 / websrv 0.34. Standalone YouTube browsing and software-decoded
playback, accounts, seeking, SponsorBlock, network preferences and TV interface.

[Beta guide](docs/BETA_V017.md) · [Console results](docs/BETA_V017_TEST_RESULTS.md)
· [Feature status](docs/BETA_FEATURE_MATRIX.md) · [Checkpoint](docs/CURRENT_WORK_CHECKPOINT.md)

Install the complete `PStube-PS5-Test` folder from the beta ZIP under
`/data/homebrew/`, then launch `eboot.elf` in the foreground through websrv.
Preserve account/config files in `/data`. 1080p is the recommended default;
native 4K and live playback remain experimental. No hardware decoding is claimed.

Build: `python3 scripts/setup.py` then `python3 scripts/build.py`.
Host checks: `./scripts/test-host.sh`. Package: `python3 scripts/package.py`.
The source/relink archive includes matching objects and dependency notices.

This source snapshot contains the PS5 application.
License: GPLv3-or-later; third-party file notices remain applicable.
