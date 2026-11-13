# PStube Beta 0.20 verification — 2026-11-14

PS5 Pro firmware13.00, OnionHEN0.9.0/websrv0.34. Base PS5 not tested.

## Owner-log diagnosis

Downloaded pre-v020/pstube-ps5-probe.log:1,951,620bytes, final clean exit. Repeated network refill waits include audio/video delays up to approximately7.7seconds. The previous video AVIO read blocked without polling controller input. Audio-start and frame-pacing waits also lacked input polling. A natural video end could join the audio worker before stopping an outstanding audio stream.

Video transport now invokes a timed main-thread wait callback outside its ring mutex. The controller can cancel/exit or select previous/next during this wait; audio-start and presentation pacing also poll cancellation. End-of-video cleanup stops the adaptive downloader and unpauses its queue before joining. AV pacing uses the audio media origin when anchoring video timestamps. These fix identified blocking paths; broader owner reproduction is still required.

## Host verification

Full host suite passes under UBSan. Added checks cover:

- Main-reader wait callback runs while a transport is starved and can safely inspect the stream outside its mutex.
- Two unselected cards receive author/like/subscriber metadata without a selection event, while image and branding jobs remain independent.
- Thumbnail icons respect the content clip with a translated canvas, leaving the sidebar unchanged.
- Storyboard selection maps a seek time to the correct sheet/crop; untrusted storyboard hosts are rejected.
- Loading spinner changes with time and clears the background.

Existing cancellation/reconnect, byte-accurate Range, metadata, UI, clock, account and playback checks remain passing. All three target ELFs pass structural validation.

## Console verification

Initial candidate search capture:20 results,16 thumbnails,8 metadata completions without changing selection. Visually checked real card image with subscriber/like/view/date information on unselected items. Clean exit.

Initial candidate playback:600 displayed/600 decoded frames, zero sync drops, no PCM underruns; clean exit. Storyboard unavailable in that response led to added signature compatibility and asynchronous WEB/TV metadata fallback in the final candidate. Preview compatibility candidate:160x90 storyboards at5-second intervals, preview images decoded for seek indices6and8.600/600 displayed frames, zero sync drops/PCM underruns, max estimated lateness13.981ms. Natural EOF check:240p15fps short VOD,284/284frames,zero sync drops; audio EOF and clean cleanup (one recorded PCM empty-queue event). These checks used the same playback/metadata/preview implementation as final; final additionally preserves Circle-to-close panels during cancellation polling.

Final executable menu check: subscriptions44/history15/liked12 videos, all with shelves=0 (flat cards).13 sections visited,20 search images loaded while moving selection100 times; clean exit.

Final playback/preview check:1080p60 VP9/Opus,600 displayed/600 decoded,zero sync drops,no PCM underruns,max estimated lateness14.805ms. Storyboard decoded successfully at indices6and8,160x90 source tiles at5-second intervals. Clean exit.

Final ELF:30,367,888bytes,SHA256 `180e13f2342ffbe2ef7d8780e65ba46cf50537db57f8bfb1f98eb493fb0b7da3`. FTP upload byte count verified. Matching objects and build manifest retained. Earlier candidate tests are separately identified by executable hash in the evidence JSON. Owner hardware checks still needed: sustained real-network stalls, physical trigger hold/repeat, paused preview cancellation, sidebar animation, varied account feeds and long live streams.

## Remaining limits

Storyboards are optional; failed/unsupported previews remain explicitly unavailable. Dislike counts are not invented. Network rejection, sustained decoder/render overload, long live-stream coverage and precise visual/audio sync still need wider owner testing. Native4K/live are experimental; HDR/hardware decoding remain unavailable.
