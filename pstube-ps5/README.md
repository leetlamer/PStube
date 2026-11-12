# PStube PS5

Native PStube beta 0.16 for the owner-tested PS5 Pro firmware 13.00 environment,
OnionHEN 0.9.0 / websrv 0.34. Standalone YouTube browsing and software-decoded
playback, accounts, seeking, SponsorBlock, network preferences and SmartTube-style UI.

[Beta guide](docs/BETA_V016.md) · [Console results](docs/BETA_V016_TEST_RESULTS.md)
· [Feature status](docs/BETA_FEATURE_MATRIX.md) · [Checkpoint](docs/CURRENT_WORK_CHECKPOINT.md)

Install the complete `PStube-PS5-Test` folder from the beta ZIP under
`/data/homebrew/`, then launch `eboot.elf` in the foreground through websrv.
Preserve account/config files in `/data`. 1080p is the recommended default;
native 4K and live playback remain experimental. No hardware decoding is claimed.

Build: `python3 scripts/setup.py` then `python3 scripts/build.py`.
Host checks: `./scripts/test-host.sh`. Package: `python3 scripts/package.py`.
The source/relink archive includes matching objects and dependency notices.

PS4 Alpha v0.53 remains preserved in `../smarttube-ps4/`; HelloCube is unchanged.
License: GPLv3-or-later; third-party file notices remain applicable.
