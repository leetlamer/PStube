# PStube PS5 beta 0.17

Requires active OnionHEN and foreground websrv launch; tested platform is PS5 Pro
firmware 13.00, OnionHEN 0.9.0 and websrv 0.34. Preserve accounts/preferences in /data.

## Playback recovery

Each adaptive audio/video stream now has an 8 MiB bounded buffer (previously 2 MiB).
Transient receive/send errors, timeouts and short responses reconnect at the next
unreceived byte using a validated HTTP206 range. Unread buffered bytes survive the
reconnect. Repeated failures stop after 8 transport retries. A server ignoring Range
is rejected rather than replaying bytes or corrupting the demuxer.

If a VOD still ends unexpectedly, the player can refresh its stream URLs and resume
near the last displayed position up to twice. User Stop/Next/Previous/Exit and normal
video completion do not trigger recovery. Live transport recovery remains separate.
Fatal/expired streams can still fail after bounded attempts; this is not a guarantee
against every network or provider error. Diagnostic logs record reconnect offsets,
media duration, premature EOF and recovery attempts without printing signed URLs.

## Decoder and memory configuration

L2 / Playback settings → Decoder threads offers 6, 8 and 10. Default 8. Audio decoding
uses 1 thread. Scaling/color conversion now uses the existing 2-thread preference in
FFmpeg rather than ignoring it. Applied decoder counts, active threading type,
scaler count and online CPU report are logged. More workers do not establish more
physical cores or guarantee faster decoding; compare 8/10 with the same video. Network,
audio output, UI and tiling also need CPU time. Memory remains bounded: two 8 MiB
stream rings, up to 64 cached 640×360 RGBA thumbnails (~56.3 MiB), display surfaces and
codec-owned reference frames. The app does not allocate unused RAM just to fill it.

## Interface

Playback controls start hidden and appear after controller input. The previous
first 120 frames forced overlay is removed. Pause/discussion and explicit debug
metrics retain their own behavior. The normal inactivity timeout remains 3 seconds.

Video cards use one rounded image/title/channel surface. Shelf cards are 460×326,
with 640×360 decoded thumbnails and three complete cards plus available neighboring
card previews. Two category rows are visible; search uses a 3×2 grid. Horizontal rows
animate over 200 ms and preserve motion continuity when navigation changes mid-slide.
Off-screen cards are clipped before the sidebar and footer. Adjacent previews are
queued with the visible images. Thumbnail detail still depends on the source image.

## HDR status

The playback settings display **HDR output: unavailable (SDR)**. This is a capability
status, not an enabled HDR mode. The verified framebuffer is 8-bit SDR; this build
has no verified HDR10/PQ/HLG display attributes, metadata submission or 10-bit output
path. HDR stream variants remain excluded to avoid washed-out/incorrect colors.
1440p/2160p source selections remain SDR. Genuine HDR needs additional platform work
and display testing; it is not implemented by raising the source-resolution limit.

## Diagnostics

--smoke-long runs a 7200-frame VOD endurance check. --smoke-reconnect forces one audio and one video transport interruption against a
real CDN; --smoke-1440-threads10 compares 10 video workers at 1440p; --browse-capture loads a public Home
feed without account authorization, hides the profile and writes a framebuffer
capture to /data/pstube-home-v017.ppm. Diagnostic overrides are not saved.

See BETA_V017_TEST_RESULTS.md for exact candidate identity and console results.
