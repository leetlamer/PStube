# Beta 0.17 feature status

| Area | Implementation | Verification |
|---|---|---|
| Category shelves | Horizontal rows, per-row cursor/continuation, modern nested headers; independent row/page pagination | Prior installed v015 console: Home2→13, Music4→11, Gaming3→11, News4→16 rows; Live server end |
| Thumbnails | 640×360 cache, rounded larger cards, animated rows and side previews | UBSan slow-branding concurrency checks; rendered UI checked |
| Stream recovery | Buffered exact-byte HTTP resume; bounded URL refresh/reseek | Host receive-error/short-body fault tests; real console forced interruptions |
| HDR output | Unavailable: current verified display path is 8-bit SDR | HDR sources excluded to prevent incorrect color |
| Selected description | Async creator metadata, clipped UTF8 right-to-left marquee | Console description1171bytes; host timing/clipping checks |
| Playback information | Actual source dimensions, fractional FPS, codec and software threads | Host actual decoder metrics; console regression |
| Navigation/playback icons | 25 embedded originals with provenance/licenses | Console imported_icons=25; host/render checked |
| Shorts | TV subscriptions shelf, Web reel fallback | Console signed-in shelf returned 15 Shorts |
| Google accounts and libraries | QR login, five slots/default/guest, subscriptions/history/playlists/channels | Owner login; console authorized browse feeds |
| Playback controls | Pause/previous/next/end mode/settings/stats; Like/Dislike/subscribe/add playlist/channel | Navigation and read-only endpoint checks; account writes need owner testing |
| Captions/dimming/discussions | JSON3 captions, dimming, read-only comments/live chat | Console captions 6 cues; comments 20; dimming; disabled chat correctly unavailable |
| DNS/proxy | App-only DoH and CONNECT/SOCKS5h proxy with verified TLS | Host persistence/validation; console Google/Cloudflare; owner proxy test pending |
| Frame pacing | Valid PTS priority; detected source-rate fallback for absent PTS | Target/host regression; HDMI refresh switching not implemented |
| SponsorBlock | Optional automatic skips, category/minimum/notice settings and coloured markers | Real console API returned6segments; all6skipped; host policy/persistence and pixel-colour checks |
| DeArrow | Independent titles/thumbnails options, trusted alternatives, original fallback; default Off | Console two title replacements; alternate image delivery unverified |
| VOD VP9/Opus, AVC/AAC | 6/8/10 video threads (8 default), 2 scaler threads, 8MiB rings and validated range reconnect | Owner varied playback and console regression; native4K remains source-dependent |
| Live AVC/AAC HLS | Rolling segments, separate audio, bounded rings | Short 1080p60 console test; longer prior tests; experimental |
| VOD seeking | Progress/preview, elapsed/duration/remaining, byte ranges, fresh AV clocks | Console forward/backward and long seeks; host keyframe/audio trimming test |
| Resume after restart, live DVR | Pending | VOD seeking implemented; persistence/live timeline work remains |
| Hardware/GPU decode, HDMI auto refresh | Not implemented | No claim of hardware acceleration |

This beta is a development milestone with the feature coverage listed above. See the
release guide and test results for limitations and exact candidate identities.
