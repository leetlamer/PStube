# PStube PS5 Beta 0.19

Testing build. Requires the existing supported OnionHEN and websrv environment. Exit PStube before copying `PStube-PS5-Test` into `/data/homebrew/`. Preserve accounts and preferences in `/data`.

## Playback buffering

Open **Settings → Playback → Buffering**, then use Left/Right or Cross. Changes apply to the next video and survive restart.

| Preset | Reservoir target | Startup target | Refill target |
|---|---:|---:|---:|
| Low | 5 seconds | 1 second | 2 seconds |
| Normal (default) | 30 seconds | 2.5 seconds | 5 seconds |
| High | 50 seconds | 4 seconds | 8 seconds |
| Maximum | 100 seconds | 6 seconds | 12 seconds |

These are compressed-byte estimates based on the selected bitrate. Actual buffered time varies; video is capped at 32 MiB and audio at 2 MiB. High bitrate streams can reach the cap before the time target. Live retains a bounded 5-second reservoir with 1-second startup and 3-second refill to avoid excessive live delay. Larger buffers cannot fix sustained CPU/render overload or HTTP403 access failures.

## Browsing and information

Standard results display four complete cards across, including search, with widths adapting as the sidebar opens or closes. Shelf end positions also account for four cards. Shorts retain portrait cards and channels retain their banner list.

Cards show channel, available views/date and thumbs-up/down counts. Selected-card metadata loads asynchronously. Playback controls show channel/subscriber information, views/date, likes/dislikes, and the existing resolution/FPS/software-decoder information. Local history and favorites preserve available information.

YouTube does not return every count for every video/client. Unreturned card counts use a dash; playback says Unavailable. Dislikes are not fabricated or inferred from likes. No additional third-party rating service is used.

Software decoding, experimental native4K/live and SDR output limitations continue to apply. See test results for verification scope. No GitHub repository or release changes were made for this testing build.
