# Contributing

PStube is a native C++17 PS5 homebrew project. Read the README, build guide and feature status
before opening an issue or proposing a change.

## Changes

- Keep changes focused and use descriptive commit subjects.
- Use four-space C++ indentation and descriptive names; follow nearby code.
- Check SDK results, keep memory bounded and preserve imported license/provenance information.
- Include relevant host checks and describe console testing separately.
- State assumptions, limitations and any behavior that still needs hardware testing.

Use a pull request against `main`. Explain the problem, resulting behavior and validation.
Do not commit SDKs, downloaded toolchains, executable packages, personal accounts, OAuth tokens,
network credentials, raw console logs or proprietary application artifacts.

## Reporting a problem

Include the PStube version, console model, firmware, OnionHEN/websrv versions and steps to
reproduce. For playback issues, include source resolution/FPS, output mode and buffer preset.
Attach only relevant, redacted diagnostic lines. Remove account identifiers, signed media URLs,
OAuth codes/tokens, proxy credentials and private network addresses before sharing.

## Testing

See [Building](docs/BUILDING.md). Portable checks help detect source regressions but do not
establish console compatibility, visual synchronization or real-time decoding performance.
