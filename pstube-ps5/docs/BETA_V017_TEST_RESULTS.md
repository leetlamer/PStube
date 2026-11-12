# Beta 0.17 test results — 2026-11-12

Test console: PS5 Pro firmware 13.00, OnionHEN 0.9.0 / websrv 0.34. SDK 0.45.
Final executable 30,077,840 bytes, SHA256:
`42ec350459b42106a452d19b33830a3f2c427a3cec7bc091c2ed9ae944298866`.
FTP byte count verified. Matching debug objects retained. PS4/cube unchanged.
Base PS5 has not been hardware-tested; this is a Pro-console result.

## Failure diagnosis

Latest owner log downloaded before updating: pre-v017/pstube-ps5-probe.log,
1,089,442 bytes. The final owner session shows an audio download receive error
(CURLE_RECV_ERROR56), audio demux EIO(-5), PCM failure and video exit after 2175
pictures without a controller cancellation. The stream had no reconnect path.
This was an application return to the menu, not a process crash.

The new transport preserves unread ring bytes and resumes after received bytes.
Local deterministic tests run the actual ring/worker and verify full payload equality
through receive errors, short successful responses and a ring wrap; a server ignoring
Range is rejected. Cancellation of a blocked full-ring producer completes promptly.
A second bounded player-level recovery refreshes URLs and seeks near the last position
when a VOD still fails or ends too early. This second path needs wider owner coverage.

## Final executable: console results

Public VOD fixture aqz-KE-bpKQ, VP9 + Opus. No account write performed. Counts below
are short 600-frame tests, not a guarantee for all video formats or full-length films.

| Test | Decoder / scaler threads | Displayed / decoded | Sync drops | Average / max flip ms |
|---|---|---|---|---|
| 1080p60 forced audio+video interruption | 8 / 2 | 600 / 600 | 0 | 16.711 / 33.357 |
| 1440p60 source → 1080p output | 8 / 2 | 600 / 600 | 0 | 16.683 / 16.705 |
| 1440p60 source → 1080p output | 10 / 2 | 600 / 600 | 0 | 16.683 / 16.713 |
| 2160p60 source → native2160p output | 8 / 2 | 600 / 600 | 0 | 16.711 / 33.364 |
| 1080p60 endurance (~120 seconds) | 8 / 2 | 7200 / 7206 | 6 | 16.686 / 33.365 |

Forced interruptions occurred at 32768 audio bytes and 524288 video bytes. Both resumed
using HTTP206 and playback continued; PCM reported no rebuffer events. The 1 frame
above 25 ms in that run was a display-interval spike, not a decoder sync drop.

Both 1440p runs met real-time 60 fps. Ten threads had no measured throughput advantage
in this sample; maximum estimated video lateness was 19.736 ms versus 12.866 ms with 8.
The 8 worker default is retained; 10 is selectable. The console reports 16 online logical
CPUs, not 16 dedicated physical cores. Audio uses 1 decoder thread. Native 4K uses 2 tiling
workers; its 207 frames more than 25 ms behind the estimated audio clock remain a timing
limitation despite zero discarded frames. Native 4K remains experimental.

Every completed playback diagnostic logged controls_start visible=0/input_triggered=0
and cleanup_complete. The launch overlay was removed; broader controller usability
and visual sync still require owner testing.

## UI and host checks

Full host UBSan suite passed, including real A/V seek/metrics, thumbnail concurrency,
paging and account/network/SponsorBlock policies. New checks verify carousel easing
continuity, clipping outside the content area and actual stream retry behavior.
The larger rounded-card preview was rendered and visually checked. Target builds
passed warnings-as-errors and structural ELF validation.

The guest-only thumbnail capture diagnostic received an empty public Home feed and
exited cleanly without producing a screenshot. No private profile or token files were
retrieved. After the owner closed normal playback, the 7200-frame endurance diagnostic completed
and exited cleanly. PCM reported zero failures/rebuffers. Six sync drops occurred
(6/7206 decoded frames); 3279 frames exceeded 25 ms estimated video lateness, maximum
35.318 ms. One display interval exceeded 25 ms. This is a stability result for about
two minutes of one 1080p60 fixture, not proof of perfect sync or full-film stability.

## HDR and remaining checks

HDR is not implemented. The verified framebuffer is 8-bit SDR; the settings show its
unavailable status. HDR input remains excluded to avoid incorrect rendering. No HDR
television, 10-bit scanout, PQ/HLG metadata or native 1440p display mode is claimed.
Test longer varied VODs/live streams, pause/resume around a network outage, seek after
recovery, rapid carousel input and thumbnail memory pressure. High-resolution timing
is source-dependent. No hardware decoder acceleration is claimed.
