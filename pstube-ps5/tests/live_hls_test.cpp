#include "live_hls.h"
#include <assert.h>
#include <stdio.h>
int main() {
 using namespace smarttube;LiveHlsVariant selected;std::string error;
 const std::string base="https://example.test/live/master.m3u8?token=fixture";
 const std::string master="#EXTM3U\n#EXT-X-STREAM-INF:CODECS=\"avc1.4d401f,mp4a.40.2\",RESOLUTION=1280x720,FRAME-RATE=30\n720/index.m3u8\n#EXT-X-STREAM-INF:CODECS=\"avc1.640028,mp4a.40.2\",RESOLUTION=1920x1080,FRAME-RATE=60\n/1080/index.m3u8\n#EXT-X-STREAM-INF:CODECS=\"vp09.00.51.08,mp4a.40.2\",RESOLUTION=1920x1080\nhttps://example.test/vp9.m3u8\n";
 assert(selectLiveHls(master,base,2160,60,selected,error) && selected.height==1080 && selected.url=="https://example.test/1080/index.m3u8");
 assert(selectLiveHls(master,base,1440,30,selected,error) && selected.height==720 && selected.url=="https://example.test/live/720/index.m3u8");
 assert(!selectLiveHls(master,base,480,30,selected,error));
 for(const auto& uri:{"http://example.test/video","file:///data/file","//example.test/video","https://name@host/video","bad\rURL"}) {
  const std::string bad="#EXTM3U\n#EXT-X-STREAM-INF:CODECS=\"avc1.4d401f,mp4a.40.2\",RESOLUTION=1280x720\n"+std::string(uri)+"\n";
  assert(!selectLiveHls(bad,base,1080,60,selected,error));
 }
 for(const char* codec:{"avc1.6e0028","avc1.640033"}) {
  const std::string bad="#EXTM3U\n#EXT-X-STREAM-INF:CODECS=\""+std::string(codec)+",mp4a.40.2\",RESOLUTION=1280x720\nhttps://example.test/video.m3u8\n";
  assert(!selectLiveHls(bad,base,1080,60,selected,error));
 }
 const std::string grouped="#EXTM3U\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aud\",NAME=\"French\",LANGUAGE=\"fr\",DEFAULT=YES,URI=\"fr.m3u8\",CHANNELS=\"2\"\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aud\",NAME=\"English\",LANGUAGE=\"en\",URI=\"en.m3u8\",CHANNELS=\"2\"\n#EXT-X-STREAM-INF:BANDWIDTH=3000000,CODECS=\"avc1.640028,mp4a.40.2\",RESOLUTION=1280x720,FRAME-RATE=30,AUDIO=\"aud\"\nvideo.m3u8\n";
 LiveHlsDiagnostics stats;
 assert(selectLiveHls(grouped,base,1080,60,selected,error,&stats,"en") && selected.audioUrl=="https://example.test/live/en.m3u8" && !selected.master.empty());
 assert(selected.master.find("BANDWIDTH=3000000")!=std::string::npos && selected.master.find("AUDIO=\"audio\"")!=std::string::npos && stats.accepted==1);
 for(const auto& detail:stats.details) assert(detail.find("https://")==std::string::npos && detail.find("token")==std::string::npos);
 const std::string inband="#EXTM3U\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aud\",NAME=\"Main\",DEFAULT=YES\n#EXT-X-STREAM-INF:CODECS=\"avc1.4d401f,mp4a.40.2\",RESOLUTION=1280x720,AUDIO=\"aud\"\nvideo.m3u8\n";
 assert(selectLiveHls(inband,base,1080,60,selected,error,&stats) && selected.audioUrl.empty() && selected.master.empty());
 const std::string media="#EXTM3U\n#EXT-X-TARGETDURATION:6\n#EXT-X-MEDIA-SEQUENCE:123\n#EXTINF:6.0,\n123.ts\n#EXTINF:6.0,\nhttps://example.test/live/124.ts\n";
 assert(validateLiveHlsMedia(media,base,error));
 LiveHlsMediaInfo layout;error="previous profile failure";
 assert(validateLiveHlsMedia(media,base,error,&layout) && error.empty());
 assert(layout.segments==2 && layout.targetSeconds==6 && layout.latestSegment=="https://example.test/live/124.ts" && !layout.fragmented && !layout.byteRanges && !layout.ended);
 assert(validateLiveHlsMedia(media+"#EXT-X-DISCONTINUITY\n#EXT-X-MAP:URI=\"init.mp4\",BYTERANGE=\"100@0\"\n#EXT-X-ENDLIST\n",base,error,&layout));
 assert(layout.fragmented && layout.byteRanges && layout.ended && layout.discontinuities==1 && layout.initialization=="https://example.test/live/init.mp4");
 assert(validateLiveHlsMedia(media+"#EXT-X-BYTERANGE:100@0\n",base,error,&layout) && layout.byteRanges);
 assert(!validateLiveHlsMedia("bad",base,error,&layout) && layout.latestSegment.empty() && !layout.segments);
 std::string ts(377,'\0');ts[0]=ts[188]=ts[376]=0x47;
 assert(std::string(liveMediaSignature(ts))=="mpeg-ts");
 ts[188]=0;assert(std::string(liveMediaSignature(ts))=="unknown");
 assert(std::string(liveMediaSignature(std::string("\0\0\0\x18" "ftyp",8)))=="iso-bmff");
 assert(std::string(liveMediaSignature(std::string("\xff\xf1",2)))=="aac-adts");
 assert(std::string(liveMediaSignature("ID3fixture"))=="id3-prefixed");
 assert(std::string(liveMediaSignature(""))=="empty");
 assert(std::string(liveMediaSignature("<html>denied</html>"))=="unknown");
 assert(!validateLiveHlsMedia(media+"#EXT-X-KEY:METHOD=AES-128,URI=\"key\"\n",base,error));
 assert(!validateLiveHlsMedia(media+"#EXT-X-MAP:URI=\"file:///bad\"\n",base,error));
 assert(!validateLiveHlsMedia("#EXTM3U\n#EXT-X-TARGETDURATION:6\n",base,error));
 assert(!validateLiveHlsMedia(std::string(512*1024+1,'a'),base,error));
 std::string dvr="#EXTM3U\n#EXT-X-TARGETDURATION:6\n";
 for(unsigned i=0;i<3000;++i) dvr+="#EXTINF:6.0,\nhttps://example.test/live/"+std::to_string(i)+".ts?signature="+std::string(160,'x')+"\n";
 assert(dvr.size()>512*1024 && validateLiveHlsMedia(dvr,base,error));
 assert(!validateLiveHlsMedia(dvr+"#EXT-X-KEY:METHOD=AES-128,URI=\"key\"\n",base,error));
 assert(!validateLiveHlsMedia(dvr+std::string(LiveHlsMediaLimit,'x'),base,error));
 puts("PASS: bounded HLS AVC/AAC selection, resolution/FPS caps, signed-base resolution, media layout and encrypted/invalid URL rejection");
}
