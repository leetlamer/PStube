# PStube Beta 0.19 testing — 2026-11-13

Console: PS5 Pro, firmware13.00, OnionHEN0.9.0 and websrv0.34. Base PS5 remains untested. Matching debug objects/build manifest retained.

## Owner log analysis

Downloaded updated log: pre-v019/pstube-ps5-probe.log,1,682,639bytes. Clean exit. Several failed starts show HTTP403 for both video/audio, zero downloaded bytes and demux EIO. Buffer settings cannot cure an access rejection. Demanding native4K sources still show substantial sync drops; buffering does not remove the CPU/render cost. One full VOD completes3,365pictures/3,388decoded,23syncdrops and one PCM underrun. Most other stopped runs have controller cancellation. No new process crash demonstrated.

## Implementation and host checks

Four-column standard results and shelf end positions now share the same visible count. Card width adapts during sidebar motion; portrait Shorts and banner channels retain their dedicated layouts. Added native listing metadata (TV tile lines, modern lockups and desktop renderers) plus asynchronous selected-item/player details. Missing counts remain unavailable; no fabricated dislike ratio.

Four saved playback buffer presets use bounded bitrate-derived targets. Full UBSan host suite passes, including existing reconnect/short EOF/Range/cancellation tests, preset bounds, saved buffer preference and TV/player metadata fields. All three target ELFs pass structural verification. Host search image checked: all four columns fit, metadata/thumbnail areas remain inside rounded cards.

## Console verification

- Search capture: HTTP200,17 results,16 thumbnails loaded. Actual console screenshot visually confirms four full cards across each of two rows, populated images, channel names, views, relative dates and duration badges. Missing likes/dislikes correctly show dashes. Clean exit.
- Category regression: Home, subscriptions, channels, history, playlists, liked, music, gaming, news, Shorts, favorites, local history and live visited. Search20 thumbnails loaded while moving selection over100frames; clean exit. No account changes made.

- Playback/controls: 1080p60 VP9/Opus,600 displayed/600 decoded,zero sync drops,no PCM underruns,max estimated video lateness20.949ms. Channel/views/date/likes/subscriber fields returned successfully; dislike count absent and remains unavailable. Comments20 entries and playlist read-only list15 returned. Clean exit.

Final ELF: 30,182,160bytes, SHA256 `e6f5bf26f4796656985ad15e415fbd959e85ee525083b3045427b4d5ae61aca2`. FTP upload byte count verified. Normal buffering policy used for console playback; all four preset bounds and persistence checked on host. Wider owner playback across each preset remains pending.

## Limits

Only returned YouTube counts can be displayed. Thumbnail enrichment is asynchronous for the selected card; listings do not include every subscriber/reaction count. Live keeps a short bounded policy. High bitrate streams hit byte limits before nominal time targets. Native4K/live remain experimental software paths, HDR unavailable. Owner testing of each preset on their affected videos remains necessary. GitHub unchanged.
