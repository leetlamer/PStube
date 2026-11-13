# Building PStube

## Status

The source tree is published as individual files. The PS5 build is verified in the existing
macOS development workspace. A complete fresh-clone dependency bootstrap is not yet available.
The repository does not contain downloaded SDKs, toolchains, proprietary console libraries,
private configuration, generated media or build artifacts.

## Console build

Keep `pstube-ps5/` beneath the repository root: scripts resolve workspace dependencies relative
to that root. Pinned dependency hashes are recorded in the application’s `third_party/` manifests.

The verified workspace uses Apple clang, a workspace `ld.lld`, community PS5 SDK 0.45,
FFmpeg 7.1.5 with the documented software codecs, and the pinned networking libraries.
Consult the setup scripts and manifests for exact paths and archive hashes.

From `pstube-ps5/`:

```sh
python3 scripts/setup.py
python3 scripts/setup-network.py
python3 scripts/build-software-codecs.py
python3 scripts/generate-fixtures.py
python3 scripts/generate-av-fixture.py
python3 scripts/build.py
```

These commands require their pinned input archives/toolchain dependencies to have been prepared.
`setup.py` reports the SDK archive URL and verifies its hash before extraction. Do not substitute
unverified SDK declarations or claim a successful host build proves firmware compatibility.

The release source/relink archive includes matching objects, library inputs, configuration and
pinned source archives for the released executable. It is a separate developer download.

## Host verification

The full macOS host suite is:

```sh
cd pstube-ps5
./scripts/test-host.sh
```

It currently expects host codec libraries in the earlier workspace layout; inspect the script
before running it on a fresh checkout. UI previews are written under `pstube-ps5/build/`.

The portable subset used by CI requires Python 3 and a C++17 compiler:

```sh
python3 scripts/check-repository.py
mkdir -p pstube-ps5/build
cd pstube-ps5
c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc \
  tests/features_v013_test.cpp src/shared/core.cpp src/shared/json.cpp \
  -o build/metadata-shelves-test
./build/metadata-shelves-test
```

## Packaging

`python3 scripts/package.py` runs from the application directory after a verified console build.
It expects matching build manifests and curated release-test evidence from the development
workspace. It creates an install ZIP and a source/relink archive, and checks their hashes and
payload layout. Release engineering inputs are not committed as raw console logs.
