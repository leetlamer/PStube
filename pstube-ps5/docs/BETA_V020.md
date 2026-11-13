# PStube PS5 Beta 0.20

Testing build for the existing OnionHEN/websrv environment. Exit the app before replacing
`/data/homebrew/PStube-PS5-Test/`. Preserve existing accounts and settings in `/data`.

- Timed controller polling during video network waits and audio-start/frame-pacing waits.
  Circle cancels, Options exits, and L1/R1 switch videos even while the video transport waits.
- Audio cleanup cancels the remaining adaptive download before joining its worker, avoiding
  waiting for an unrelated/longer audio stream after video playback ends.
- Playback transitions clear to black with a centred animated spinner. Paused overlays restore
  a clean picture before drawing, preventing accumulated preview/control artifacts.
- Visible cards queue metadata automatically on an independent worker; selection is not required.
  Original thumbnails have separate workers. Missing metadata remains unavailable.
- Thumbnail icons now obey content clipping and cannot draw over the sidebar.
- Subscriptions, History and Liked videos use four-column cards, rather than horizontal shelves.
- Seek controls show a second, smaller storyboard frame when YouTube provides a valid storyboard.
  Unsupported/live videos or failed preview requests show Preview unavailable.
- L2/R2 move the seek selection backward/forward by 30 seconds, repeating while held. After
  two seconds, steps increase to 60 seconds. Cross confirms; Circle cancels the selection.
  D-pad seeking retains 10-second steps.
- Generic Channel labels no longer overwrite a known creator name; card metadata can fall back
  to the public oEmbed author field.

Buffer presets remain under Settings → Playback. 1080p is the performance baseline. Native4K
and live are experimental software paths; HDR, hardware decoding and HDMI refresh switching
remain unavailable. Network rejections and sustained CPU overload remain separate limitations.

See the matching test results for verification and remaining owner checks.
