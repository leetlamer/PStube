# Changelog

## 0.20 — 2026-11-14

- Keep playback controls responsive during video reads and audio-start/pacing waits; cancel remaining adaptive audio before joining.
- Clear transitions with an animated spinner and restore clean paused frames before overlays.
- Populate visible-card metadata without selection; clip icons to prevent sidebar overlap.
- Restore four-column Subscriptions, History and Liked Videos.
- Add optional storyboard seek previews and accelerated L2/R2 seek selection.
- Preserve known creator names when upstream returns a generic Channel label.
- Update the README with current console screenshots.


## 0.19 — beta

Changes since the published 0.17 beta:

- Add bounded startup/refill buffering and independent audio-clock handling during video starvation.
- Add persisted Low, Normal, High and Maximum VOD buffer presets; protect the shorter live policy.
- Show four complete landscape cards across search/results and correct shelf end positions.
- Add a sliding sidebar, vertical scrolling, consistent account-feed cards and portrait Shorts.
- Move settings into a dedicated hub and fix playback Back navigation.
- Restore available channel, view/date, like and subscriber information on cards/playback.
- Keep unavailable counts explicit; preserve available local history/favorite metadata.

Native 4K/live remain experimental. HDR, hardware decoding and HDMI refresh switching are
not implemented. Some streams can still be rejected with HTTP403.

## 0.17 — beta

Published baseline with bounded HTTP reconnection, selectable decoder threads, rounded cards,
animated shelves, playback information and source/relink release materials.
