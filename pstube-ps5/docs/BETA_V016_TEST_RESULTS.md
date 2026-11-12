# Beta 0.16 test results — 2026-11-12

PS5 Pro, owner firmware 13.00, OnionHEN 0.9.0 / websrv 0.34, SDK 0.45.
Software decoding. Final ELF: 29,969,296 bytes. SHA256:
`c43fd352d05d76e50b7a5d4e3b1ec99e0108d32680bd8113b9ecf4293372c027`.
FTP upload exact byte count verified; matching debug ELF and objects retained.
Earlier releases preserved. No owner account/config files downloaded or altered by
diagnostic overrides. Diagnostics completed cleanup before subsequent launches.

## Latest owner log review

Downloaded the append-only console log before updating (pre-v016, 898,679 bytes).
The prior installed v0.15 session reached 20 named Home sections / 110 videos and
ended normally. Other recorded sessions show both clean VOD playback and large
frame-drop counts at native 4K. This UI beta does not retune the decoder or claim a
4K performance improvement. Pagination is bounded and follows server continuations;
a feed with no continuation is not evidence of a client loading failure.

## Console checks on final ELF

`--dearrow-test`: 25 imported icons loaded, two original-image workers plus one
branding worker. Creator description loaded via TV next HTTP200: 1,171 bytes.
Three branding requests completed; two responses and two alternate titles applied.
No alternate image applied. Original preview fallback retained. Browser inspection
confirmed the public fixture has a trusted thumbnail and a 1280×720 image endpoint;
the console image failure's precise network/format cause is not established.
**Replacement thumbnails remain an unverified beta feature.** No votes/submissions.

`--browse-test`: named headers remained present while additional category pages
and per-row videos were appended. Final results vary with YouTube responses:

| Feed | Initial → final rows | Initial → final videos | More page data available |
|---|---|---|---|
| Home | 2 → 11 | 10 → 71 | Yes |
| Music | 3 → 9 | 18 → 59 | Yes |
| Gaming | 3 → 14 | 72 → 290 | No |
| News | 4 → 16 | 20 → 85 | Yes |
| Live | 3 → 3 | 15 → 15 | No |

`--smoke-features`: TV player returned UNPLAYABLE/reload, normal VISIONOS fallback
resolved 1080p60 VP9 + separate Opus. Displayed 600/600 decoded pictures, zero sync
drops, zero video frames over 25 ms late, PCM rebuffer events zero. This is a short
smoke test, not a long-duration performance guarantee. Output 1920×1080, average
flip interval 16.683 ms, maximum 16.710 ms, zero flip intervals above 25 ms.
Imported controls exercised dimming, captions availability (this source had none),
20 read-only comments and 15 editable-playlist listings. No account write executed.
All three diagnostics reached cleanup_complete and returned to the loader.

## Local validation

Full host suite passed under UBSan: graphics/bounds, video layout, decoders, A/V and
seek clocks, actual stream metrics, paging append/dedup/row offsets, accounts,
network preferences, SponsorBlock policy and DeArrow trust/fallback. Slow branding
fixture proves original images can complete while optional branding is pending.
Target builds passed warnings-as-errors and structural ELF verification.
UI preview visually checked: larger cards, three rows/four columns, no title/channel
overlap, alpha-blended imported icons. Focused marquee check confirms right-to-left
motion and clipping with UTF8 text. Description parser covers TV panels and fallback.

Evidence hashes and extracted current sessions are in the source/relink archive.
The install ZIP verifies all SHA256SUMS entries, fixture hashes, owner icon and ELF.

## Owner checks before broad release

Check readability/marquee speed on the TV, card selection while images arrive,
DeArrow mode persistence and image replacement on varied videos, longer VOD/live
sessions and seeking. Native 4K remains experimental. Hardware decoding and HDMI
refresh switching are not implemented. Existing SponsorBlock/seek controls retained;
see beta 0.14 results for the earlier real six-range skip test.
