# PStube PS5 beta 0.13

Standalone native PS5 app, preserving the working software decoder and 1080p/1440p/
2160p selection. Install the complete PStube-PS5-Test folder in /data/homebrew/ and
launch its eboot.elf in foreground through OnionHEN/websrv. Account files and saved
preferences stay in /data; the release does not replace them. Supplied blue icon retained.

## Browsing

Thumbnail downloads and decoding now run on two persistent workers with connection
reuse. Movement, selection, sidebar and search remain available while images arrive.
A bounded64-image cache stores384x216 previews; stale requests cannot populate a new
page. Failed images retry after a cooldown. Shorts titles can fill in asynchronously.

Home, Music, Gaming, Live and News use category shelves: Left/Right scrolls the active
row, Up/Down changes category, and each row remembers its position. Approaching the
end of a row loads its continuation when supplied by YouTube. Rows are provided by
the service, so their number and titles vary. Shorts first uses the signed-in TV
subscriptions shelf, falling back to the Web reel feed. Home loads on startup rather
than restoring an old search. Search and account libraries retain the smaller grid;
Channels keeps six compact banners. Triangle opens search, Square saves a favorite,
R1 loads more, L2 opens settings, Circle switches sidebar and OPTIONS exits.

## Playback

UP focuses the two-row control panel. D-Pad chooses a button; CROSS activates it.
Circle hides controls first and then returns to browsing. Controls hide after idle
while video runs. Cross without control focus pauses/resumes. L1/R1 previous/next;
Square/Triangle opens playback settings. Account actions briefly pause audio/video
while YouTube responds; comments/chat fetching continues in the background.

Primary row: play/pause, previous, next, settings, stats, auto-next/loop/stop.
Secondary row: Like, Dislike, Channel, CC, Add to playlist, Subscribe, Comments/Chat,
picture dimming. Channel name and icon appear above the controls when available.
Channel opens its videos. Ratings and subscriptions toggle only after the server
accepts the request. Add to playlist opens the account's editable playlists and
Cross adds the current video. These actions require a linked account.

CC cycles available caption tracks and Off; the selected track label appears in a
toast. JSON3 captions follow the video's media timestamp. Some videos have no tracks,
and some clients may not return usable caption data. The button reports that case.
Picture dimming cycles0/25/50/75%; it uses the existing color conversion, changes the
video picture and leaves controls readable. It does not change TV brightness settings.

Comments and live chat are read-only. UP/DOWN scrolls, R1/Cross fetches more, L1 switches
comments/chat, Circle closes the panel. Live chat refreshes while open when the
broadcast provides a continuation. Disabled/missing discussions are reported; the
app does not invent messages or send comments/chat. Availability varies by broadcast.

## Verification and remaining work

See BETA_V013_TEST_RESULTS.md for console results and exact release hashes. Automated
checks do not alter account ratings, subscriptions or playlist contents. Owner testing
is still needed for those writes, subjective audio sync and visual controller layout.
Seeking/resume, automatic SponsorBlock skipping, comment posting, reply trees/moderation, live-chat replay
and full SmartTube parity remain pending. Live buffering and high-resolution software
performance remain source-dependent; native4K output is experimental and opt-in.

Source design reference: SmartTube32.72 commit a945135eee6387cd6549b87604ad673944c06b5d,
MediaServiceCore db543d4d5d639cbf3084e3870b0dac6e4e2524a1, retrieved2026-11-07.
No Android runtime or proprietary decoder libraries are required.

## Network settings

L2 / Settings → Network settings (from browsing). App-only DNS choices are System,
Google DNS-over-HTTPS, Cloudflare DNS-over-HTTPS and a custom HTTPS DoH endpoint.
Proxy choices are Off, HTTP CONNECT, HTTPS CONNECT and SOCKS5 with remote DNS.
Enter the server, port and optional credentials; Test checks the draft route before
Save. A proxy resolves destination names remotely, so DoH is not combined with it.
TLS verification stays enabled. Existing transfers finish on their original route;
new requests use the saved setting. Network preferences are stored privately in
/data/pstube-ps5-network.json. Console-wide DNS settings are unchanged. Google and
Cloudflare passed console tests; a user-provided proxy still needs owner testing.

## Frame pacing and optional content services

Frame timestamps remain authoritative. Missing timestamps now advance from the last
video timestamp using the stream's detected frame rate, preserving fractional rates,
instead of assuming 60fps. This is automatic. It does not switch the TV's HDMI refresh
mode: SmartTube uses Android display APIs for that function and no verified PS5
counterpart is implemented. A 24/25/30fps source on a 60Hz display can still show cadence
judder. A missing-timestamp fix cannot overcome a decoder/scaler throughput limit.

SponsorBlock and DeArrow are Off by default; enable them in the playback settings.
They send video IDs to sponsor.ajay.app without Google authentication credentials.
DeArrow also requests images from dearrow-thumb.ajay.app. Changes apply to the next
video or newly loaded browse page. Restarting with them enabled retains the choice.

SponsorBlock currently **mutes sponsor audio**, using the community's sponsor segment
times. It does not skip video, mute other categories or submit votes. Live streams are
excluded. Failed/unavailable segment requests leave playback unchanged. Segments
arriving after playback starts only affect audio that has not yet been submitted.
Automatic skipping requires seekable streaming and remains pending.

DeArrow fetches community titles and thumbnails on background thumbnail workers,
after queuing the original image. Only non-original alternatives with nonnegative
votes or a locked flag are accepted. Missing/failed alternatives preserve the original.
Optional requests can slow how quickly subsequent thumbnails arrive; leave Off when
comparing baseline thumbnail throughput. Data provided by SponsorBlock/DeArrow:
https://sponsor.ajay.app and https://dearrow.ajay.app; API documentation and attribution
requirements: https://wiki.sponsor.ajay.app/w/API_Docs and
https://wiki.sponsor.ajay.app/w/API_Docs/DeArrow.
