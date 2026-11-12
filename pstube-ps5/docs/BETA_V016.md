# PStube PS5 beta 0.16

This beta combines the pending 0.15 pagination/decoder-info fixes with larger cards,
SmartTube icons, scrolling descriptions and improved DeArrow integration. Install
the complete PStube-PS5-Test folder in /data/homebrew/ and launch eboot.elf through
OnionHEN/websrv in foreground. Existing Google accounts and preferences stay in /data.
The owner's glossy blue PStube application icon remains unchanged.

## Browsing and selected video information

Home, Music, Gaming, News and Live keep horizontal category rows. RIGHT near a row's
end requests more videos for that row; DOWN near the last rows requests more category
sections. R1 requests more/retries. Row and page continuation tokens are separate.
The app displays the rows and videos returned by YouTube, rather than inventing a
fixed number. Feeds have bounded limits of 40sections/1000videos per loaded page;
the image cache holds 64previews. Reopen a section to refresh its feed.

Shelf previews are taller (180px instead of 160px) and grid images slightly larger.
Three rows and four columns remain visible. Selected long card titles scroll right
to left after a short delay. The footer shows the full selected title and a scrolling
creator description. Selection changes restart the scroll. Descriptions load after
a 0.8 second dwell on a video, asynchronously, with a bounded session cache. Missing
metadata leaves the normal status text visible. No Google account is needed to read
public descriptions. Long descriptions are bounded at 32 KiB; line breaks become spaces
in the ticker. Channel/playlist lists do not issue video-description requests.

## Imported SmartTube assets

Twenty-five navigation/playback icons are copied unchanged from the pinned SmartTube
source and embedded in the executable. They include Home, Search, Subscriptions,
Channels, Music, Gaming, Live, News, Shorts and common playback actions. PNG originals,
SHA256 provenance and applicable MIT/Apache2.0 licenses are supplied under
assets/smarttube/. PStube keeps its blue theme and its own application branding.
No Android UI runtime or plugin binary is required.

## DeArrow

L2 / Settings → DeArrow cycles **Off**, **Titles + thumbnails**, **Titles only** and
**Thumbnails only**; LEFT reverses the cycle. Choices persist and apply to newly
loaded cards; reopen a section after changing the choice. Default is Off.

Like SmartTube, PStube requests community alternatives for a video, accepts trusted
non-original suggestions and preserves originals when no suitable data is returned.
Two workers load original images; a separate worker handles optional DeArrow API
and image requests so alternatives do not occupy the original-image workers. Navigation
remains responsive. Accepted alternate URLs are remembered within the
current browse page for image-cache reloads. There is no claim that every video has
an alternative or that generated images are always available.

The console test applied two alternate titles. Replacement-image delivery remains
unverified on the console; originals remain visible when it fails.

DeArrow receives public video IDs at sponsor.ajay.app; alternate images come from
dearrow-thumb.ajay.app. Google credentials are not sent to these services. PStube
sends no submissions or votes and bundles no community data. Attribution: DeArrow,
https://dearrow.ajay.app. Public API documentation:
https://wiki.sponsor.ajay.app/w/API_Docs/DeArrow.

## Playback and limits

The playback header again shows actual source dimensions, detected fractional FPS,
codec and software decoder thread count. Imported icons replace several drawn action
symbols. The seek bar/legend now fits within the lower playback panel. Seeking,
SponsorBlock categories/skipping, captions, accounts and DNS/proxy settings are retained;
see BETA_V014.md and BETA_V013.md for their controls and limitations.

1080p display output remains the default for new preferences. Existing owner settings
are retained. Native 4K is experimental and source-dependent; this build does not add
hardware decoding, HDMI refresh switching or live DVR. Test evidence and remaining
owner checks are listed in BETA_V016_TEST_RESULTS.md.
