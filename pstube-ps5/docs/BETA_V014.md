# PStube PS5 beta 0.14

Includes beta0.13 browsing, account features, captions, discussions, DeArrow and
app-only DNS/proxy settings. Install the complete PStube-PS5-Test folder under
/data/homebrew/ and launch eboot.elf through OnionHEN/websrv in foreground. Existing
account/preferences files stay in /data. Supplied blue icon remains unchanged.

## Seek and progress bar

For normal videos, press LEFT/RIGHT during playback to open the seek bar and move
its preview by10seconds. Repeat to go farther; CROSS commits, CIRCLE cancels. UP
opens the existing two-row controls; DOWN through those rows also reaches the seek
bar. It displays the selected/current time, full duration and time remaining.
Live broadcasts show LIVE and currently do not support DVR/time shifting.

Seeking starts fresh video/audio demuxers and decoder queues. The engine requests
byte ranges, seeks to a preceding keyframe, discards earlier video and trims audio
to the selected time. It resets playback clocks together. It does not download the
whole video first. A seek can pause for network/index/keyframe processing; speed
varies by source and connection. Pause state resumes playback when a seek is committed.
Caption selection/cues and picture dimming are retained through the seek.

## SponsorBlock settings and coloured markers

L2 / Settings → SponsorBlock settings (also available through playback settings):

- Automatic skipping: On/Off. Default Off; enabling the old beta0.13 mute option
  now enables skipping, preserving the owner's opt-in choice.
- Sponsors, self promotion, introductions, outros, interaction reminders: Skip/Keep.
  Default skips only Sponsors.
- Minimum segment length:0/1/3/5seconds.
- Skip notices: On/Off. Default On.

Settings persist across restarts and apply to the next video. SponsorBlock skips
only enabled categories and only API segments with actionType=skip. Overlapping
segments are merged for seeking; incorrect-duration/invalid/too-short entries are
ignored. End-of-video segments finish playback and follow auto-next/loop/stop.
SponsorBlock is integrated natively through its public API; no browser extension
or Android plugin is installed. Community coverage and timing accuracy vary.

When controls show the progress bar, detected enabled segments are coloured:
**green** sponsors, **yellow** self promotion, **cyan** intros, **blue** outros,
**pink** interaction reminders. The legend names detected categories. The white
playhead/preview remains separate from these markers. No-data/API failures leave
normal playback available and produce no invented markers or skips.

Requests send the public video ID and selected categories to sponsor.ajay.app,
without Google credentials. No submissions, votes or skipped-view telemetry are
sent. Service and community data attribution: SponsorBlock, https://sponsor.ajay.app.
API documentation: https://wiki.sponsor.ajay.app/w/API_Docs.

## Limits and tests

See BETA_V014_TEST_RESULTS.md for exact console results and hashes.1080p output is
the default for new preferences; existing quality/output preferences are retained.
Native4K remains experimental/source-dependent. Hardware/GPU decoding and HDMI
refresh-rate switching are not implemented. Live remains experimental; this release
adds VOD seeking without enabling unsupported live DVR behavior.
