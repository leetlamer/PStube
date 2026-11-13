# Third-party notices

PStube is licensed under GPL-3.0-or-later. Existing copyright, permission and patent
notices remain applicable to the files and assets to which they refer.

- PS5 community SDK 0.45: https://github.com/ps5-payload-dev/sdk. Pinned sources and
  hashes are recorded in `pstube-ps5/third_party/provenance.json`.
- SDL-derived video tiling/address calculations: retain the notices in `src/tile.h`
  and `pstube-ps5/third_party/SDL-LICENSE.txt` (zlib license).
- FFmpeg 7.1.5: LGPL-2.1-or-later, LGPL-only configuration. Release source/relink
  archive includes the pinned source archive, build configuration and matching
  objects/static libraries for relinking. See `decoder-build-manifest.json`.
- Networking: curl, OpenSSL, libpsl, Zstandard and zlib. Exact versions/library hashes
  are in `pstube-ps5/third_party/network-manifest.json`; their license texts are in
  `pstube-ps5/third_party/licenses/` and the install ZIP's `licenses/` directory.
- Gontserrat font: SIL Open Font License 1.1; see `pstube-ps5/assets/OFL.txt`.
- stb image/font helpers: retain their source notices and `assets/stb-LICENSE.txt`.
- Imported navigation/action icons: copyright (c) 2020-present yuliskov, MIT license;
  Android Leanback playback icons, Apache-2.0. Original notices and per-file provenance
  remain alongside the imported icons in `pstube-ps5/assets/`. No Android runtime is
  linked. PStube's application icon/logo is the owner's supplied artwork.
- Optional community services: SponsorBlock (https://sponsor.ajay.app) and DeArrow
  (https://dearrow.ajay.app). Data is fetched at runtime, not bundled; their respective
  data terms apply. API documentation: https://wiki.sponsor.ajay.app/w/API_Docs.

YouTube thumbnails and metadata in the Gaming and playback screenshots were returned by the service
at capture time. They identify the displayed videos and are not PStube artwork.
PStube is independent and is not affiliated with Sony or Google.
