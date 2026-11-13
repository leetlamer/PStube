// Native adaptations of SmartTube presentation/media concepts. See NOTICE.md.
#include "core.h"
#include <algorithm>
#include <math.h>
#include <stdlib.h>

namespace smarttube {
namespace {
int bounded(int value,int minimum,int maximum) { return std::max(minimum,std::min(value,maximum)); }
std::string text(const Json* node) {
    if(!node) return {};
    if(node->type==Json::String) return node->text;
    std::string result=node->string("simpleText");
    if(result.empty()) result=node->string("content");
    if(!result.empty()) return result;
    if(const auto* runs=node->get("runs")) for(const auto& run:runs->children) result+=run.string("text");
    return result;
}
const Json* path(const Json* node,std::initializer_list<const char*> parts) {
    for(const char* part:parts) { if(!node) return nullptr;node=node->get(part); }
    return node;
}
bool liveBadge(const Json& node,unsigned depth=0) {
    if(depth>12) return false;
    const auto style=node.string("style");
    if(style=="LIVE" || style=="BADGE_STYLE_TYPE_LIVE_NOW" || node.string("iconType")=="LIVE") return true;
    if(const auto* flag=node.get("isLive")) if(flag->type==Json::Bool && flag->text=="true") return true;
    for(const auto& child:node.children) if(liveBadge(child,depth+1)) return true;
    return false;
}
void videos(const Json& node,std::vector<Video>& output,unsigned depth) {
    if(depth>64 || output.size()>=200) return;
    auto id=node.string("videoId");
    const Json* title=node.get("title");
    const Json* thumbnail=node.get("thumbnail");
    // SmartTube MediaServiceCore TileItem paths: TV search/browse tiles differ
    // from the desktop videoRenderer schema. Do not treat channel tiles as videos.
    if(const auto* watch=path(&node,{"onSelectCommand","watchEndpoint"})) id=watch->string("videoId");
    if(id.empty()) if(const auto* reel=path(&node,{"onSelectCommand","reelWatchEndpoint"})) id=reel->string("videoId");
    // CommonItems.kt LockupItem: require a watch command, not contentId alone
    // (playlists and channels also have content IDs).
    if(id.empty()) if(const auto* command=path(&node,{"rendererContext","commandContext","onTap","innertubeCommand","watchEndpoint"})) id=command->string("videoId");
    if(!title) title=path(&node,{"metadata","lockupMetadataViewModel","title"});
    if(!thumbnail) thumbnail=path(&node,{"contentImage","thumbnailViewModel","image"});
    if(!title) title=path(&node,{"metadata","tileMetadataRenderer","title"});
    const Json* header=path(&node,{"header","tileHeaderRenderer"});
    if(!header) header=path(&node,{"header","trackTileHeaderRenderer"});
    // Current SmartTube V4 TV search places metadata on thumbnail overlays.
    // Inspect every overlay: time badges may precede the metadata renderer.
    if(!title && header) if(const auto* overlays=header->get("thumbnailOverlays"))
        for(const auto& overlay:overlays->children) {
            title=path(&overlay,{"tileMetadataRenderer","title"});
            if(title) break;
        }
    if(!title && header) title=header->get("title");
    if(!thumbnail && header) thumbnail=header->get("thumbnail");
    if(!id.empty() && title && !text(title).empty()) {
        bool duplicate=false;for(const auto& old:output) if(old.id==id) duplicate=true;
        if(!duplicate) {
            Video video{id,text(title),text(node.get("longBylineText")),{}};
            if(video.channel.empty()) video.channel=text(node.get("shortBylineText"));
            video.views=text(node.get("viewCountText"));if(video.views.empty()) video.views=text(node.get("shortViewCountText"));
            video.published=text(node.get("publishedTimeText"));
            video.badge=text(node.get("lengthText"));
            if(video.badge.empty() && header) video.badge=text(header->get("duration"));
            if(video.badge.empty() && header) if(const auto* overlays=header->get("thumbnailOverlays"))
                for(const auto& overlay:overlays->children) {video.badge=text(path(&overlay,{"thumbnailOverlayTimeStatusRenderer","text"}));if(!video.badge.empty()) break;}
            if(video.badge.empty()) if(const auto* overlays=path(&node,{"contentImage","thumbnailViewModel","overlays"})) {
                for(const auto& overlay:overlays->children) {
                    const auto* badges=path(&overlay,{"thumbnailOverlayBadgeViewModel","thumbnailBadges"});
                    if(!badges) badges=path(&overlay,{"thumbnailBottomOverlayViewModel","badges"});
                    if(badges) for(const auto& badge:badges->children) {if(const auto* value=badge.get("thumbnailBadgeViewModel")) video.badge=value->string("text");if(!video.badge.empty()) break;}
                    if(!video.badge.empty()) break;
                }
            }
            if(liveBadge(node)) video.badge="LIVE";
            if(video.badge.size()>16) video.badge.clear();
            if(video.channel.empty()) {
                if(const auto* rows=path(&node,{"metadata","lockupMetadataViewModel","metadata","contentMetadataViewModel","metadataRows"})) {
                    for(const auto& row:rows->children) if(const auto* parts=row.get("metadataParts")) {
                        for(const auto& part:parts->children) {const auto value=text(part.get("text"));if(!value.empty()) {if(!video.channel.empty()) video.channel+=" · ";video.channel+=value;}}
                        if(!video.channel.empty()) break;
                    }
                }
            }
            if(header) if(const auto* overlays=header->get("thumbnailOverlays")) for(const auto& overlay:overlays->children) if(const auto* metadata=overlay.get("tileMetadataRenderer")) {
                if(video.channel.empty()) video.channel=text(metadata->get("line1"));
                auto details=text(metadata->get("line2"));if(video.views.empty() && !details.empty()) video.views=details;
            }
            if(const auto* rows=path(&node,{"metadata","lockupMetadataViewModel","metadata","contentMetadataViewModel","metadataRows"})) if(rows->children.size()>1) {
                if(const auto* parts=rows->children[1].get("metadataParts")) {if(video.views.empty() && !parts->children.empty()) video.views=text(parts->children[0].get("text"));if(video.published.empty() && parts->children.size()>1) video.published=text(parts->children[1].get("text"));}
            }
            const auto tileLines=[&](const Json* metadata) {
                if(!metadata) return;const auto* lines=metadata->get("lines");if(!lines) return;
                std::string summary;
                for(const auto& line:lines->children) if(const auto* parts=path(&line,{"lineRenderer","items"})) for(const auto& part:parts->children) if(const auto* entry=part.get("lineItemRenderer")) {
                    auto value=text(entry->get("text"));if(value.empty() || value=="•" || value=="·") continue;
                    const auto channelLink=[&](const auto& self,const Json& n,unsigned depth)->bool {if(depth>12) return false;if(n.string("browseId").compare(0,2,"UC")==0) return true;for(const auto& child:n.children) if(self(self,child,depth+1)) return true;return false;};
                    if(channelLink(channelLink,*entry,0)) {if(video.channel.empty()) video.channel=value;}else {if(!summary.empty()) summary+=" · ";summary+=value;}
                }
                if(video.views.empty()) video.views=summary;
            };
            tileLines(path(&node,{"metadata","tileMetadataRenderer"}));
            if(header) if(const auto* overlays=header->get("thumbnailOverlays")) for(const auto& overlay:overlays->children) tileLines(overlay.get("tileMetadataRenderer"));
            if(thumbnail) {
                const auto* list=thumbnail->get("thumbnails");
                if(!list) list=thumbnail->get("sources");
                int chosenWidth=0;
                if(list) for(const auto& thumb:list->children) {
                    const auto url=thumb.string("url");const int width=thumb.integer("width");
                    if(url.compare(0,8,"https://") || width>1280) continue;
                    if(video.thumbnail.empty() || (width>=480 && (chosenWidth<480 || width<chosenWidth)) || (chosenWidth<480 && width>chosenWidth)) {video.thumbnail=url;chosenWidth=width;}
                }
            }
            output.push_back(video);
        }
    }
    for(const auto& child:node.children) videos(child,output,depth+1);
}
void playlists(const Json& node,std::vector<Video>& output,unsigned depth) {
    if(depth>64 || output.size()>=200) return;
    std::string id=node.string("playlistId");
    if(id.empty()) if(const auto* watch=path(&node,{"onSelectCommand","watchPlaylistEndpoint"})) id=watch->string("playlistId");
    if(id.empty() && node.string("contentType")=="TILE_CONTENT_TYPE_PLAYLIST") if(const auto* watch=path(&node,{"onSelectCommand","watchEndpoint"})) id=watch->string("playlistId");
    const Json* endpoint=path(&node,{"onSelectCommand","browseEndpoint"});
    if(!endpoint) endpoint=path(&node,{"navigationEndpoint","browseEndpoint"});
    if(!endpoint) endpoint=path(&node,{"rendererContext","commandContext","onTap","innertubeCommand","browseEndpoint"});
    std::string browse=endpoint?endpoint->string("browseId"):std::string();
    if(browse.empty() && !id.empty()) browse="VL"+id;
    const Json* title=node.get("title");
    if(!title) title=path(&node,{"metadata","tileMetadataRenderer","title"});
    if(!title) title=path(&node,{"header","tileHeaderRenderer","title"});
    if(!title) title=path(&node,{"metadata","lockupMetadataViewModel","title"});
    if(!title) if(const auto* overlays=path(&node,{"header","tileHeaderRenderer","thumbnailOverlays"})) for(const auto& overlay:overlays->children) {title=path(&overlay,{"tileMetadataRenderer","title"});if(title) break;}
    if(browse.compare(0,2,"VL")==0 && browse.size()<=256 && !text(title).empty()) {
        bool duplicate=false;for(const auto& item:output) if(item.browseId==browse) duplicate=true;
        if(!duplicate) {Video item{browse,text(title),"Playlist",""};item.browseId=browse;item.badge="Playlist";
            const Json* thumbnail=node.get("thumbnail");if(!thumbnail) thumbnail=path(&node,{"header","tileHeaderRenderer","thumbnail"});if(!thumbnail) thumbnail=path(&node,{"contentImage","thumbnailViewModel","image"});
            if(thumbnail) {const Json* list=thumbnail->get("thumbnails");if(!list) list=thumbnail->get("sources");if(list) for(const auto& image:list->children) {const auto url=image.string("url");if(url.compare(0,8,"https://")==0 && image.integer("width")<=1280) {item.thumbnail=url;break;}}}
            output.push_back(std::move(item));}
    }
    for(const auto& child:node.children) playlists(child,output,depth+1);
}
void channels(const Json& node,std::vector<Video>& output,unsigned depth) {
    if(depth>64 || output.size()>=200) return;
    const Json* endpoint=path(&node,{"onSelectCommand","browseEndpoint"});
    if(!endpoint) endpoint=path(&node,{"navigationEndpoint","browseEndpoint"});
    if(!endpoint) endpoint=path(&node,{"endpoint","browseEndpoint"});
    if(!endpoint) endpoint=path(&node,{"rendererContext","commandContext","onTap","innertubeCommand","browseEndpoint"});
    const auto id=endpoint?endpoint->string("browseId"):node.string("channelId");
    const Json* title=node.get("title");
    if(!title) title=path(&node,{"metadata","tileMetadataRenderer","title"});
    if(!title) title=path(&node,{"header","tileHeaderRenderer","title"});
    if(!title) title=path(&node,{"metadata","lockupMetadataViewModel","title"});
    const auto params=endpoint?endpoint->string("params"):std::string();
    if((id.compare(0,2,"UC")==0 || (id=="FEsubscriptions" && !params.empty())) && id.size()<=64 && params.size()<=2048 && !text(title).empty()) {
        bool exists=false;for(const auto& item:output) if(item.browseId==id && item.browseParams==params) exists=true;
        if(!exists) {
            Video item{id+params,text(title),"Channel",""};item.browseId=id;item.browseParams=params;item.badge="Channel";
            const Json* thumbnail=node.get("banner");if(!thumbnail) thumbnail=node.get("thumbnail");if(!thumbnail) thumbnail=path(&node,{"header","tileHeaderRenderer","thumbnail"});
            if(thumbnail) if(const auto* list=thumbnail->get("thumbnails")) for(const auto& image:list->children) {auto url=image.string("url");if(url.compare(0,2,"//")==0) url="https:"+url;if(url.compare(0,8,"https://")==0 && image.integer("width")<=1280) {item.thumbnail=url;}}
            output.push_back(std::move(item));
        }
    }
    for(const auto& child:node.children) channels(child,output,depth+1);
}
std::string continuation(const Json& node,unsigned depth) {
    if(depth>64) return {};
    if(const auto* command=node.get("continuationCommand")) {const auto token=command->string("token");if(token.size()<=16384) return token;}
    if(const auto* next=node.get("nextContinuationData")) {const auto token=next->string("continuation");if(token.size()<=16384) return token;}
    for(const auto& child:node.children) {auto token=continuation(child,depth+1);if(!token.empty()) return token;}
    return {};
}
std::string context(const std::string& client,const std::string& version) {
    return "\"context\":{\"client\":{\"clientName\":"+quoteJson(client)+",\"clientVersion\":"+quoteJson(version)+",\"hl\":\"en\",\"gl\":\"US\"}}";
}
Request request(const char* endpoint,const std::string& fields,const std::string& client,const std::string& version) {
    return {std::string("https://www.youtube.com/youtubei/v1/")+endpoint,"{"+context(client,version)+","+fields+"}",
            "Mozilla/5.0 (PlayStation 4; SmartTube PS4 development)",client,version};
}
}
std::vector<Video> channelCatalog(const Json& response) {std::vector<Video> result;channels(response,result,0);return result;}
std::vector<Video> playlistCatalog(const Json& response) {std::vector<Video> result;playlists(response,result,0);return result;}
std::string catalogContinuation(const Json& response) {return continuation(response,0);}
DisplayLayout layout(int width,int height,float overscan) {
    // A uniform 1080p design grid; 4K layout support is independent of video decode support.
    DisplayLayout out;
    if(width<640 || height<480 || width>3840 || height>2160) { width=1920;height=1080; }
    out.outputWidth=width;out.outputHeight=height;
    out.scale=std::min(width/1920.0f,height/1080.0f);
    const auto scaled=[&](float value) { return std::max(1,(int)lroundf(value*out.scale)); };
    overscan=isfinite(overscan)?std::max(0.0f,std::min(overscan,0.15f)):0.05f;
    const int marginX=(int)lroundf(width*overscan),marginY=(int)lroundf(height*overscan);
    out.safe={marginX,marginY,width-2*marginX,height-2*marginY};
    out.gap=scaled(24);out.fontPixels=std::max(12,scaled(30));
    const int side=std::min(scaled(250),out.safe.w/3);
    out.sidebar={out.safe.x,out.safe.y,side,out.safe.h};
    out.content={out.safe.x+side+out.gap,out.safe.y,std::max(1,out.safe.w-side-out.gap),out.safe.h};
    out.columns=bounded((out.content.w+out.gap)/(scaled(290)+out.gap),2,6);
    out.cardWidth=std::max(1,(out.content.w-(out.columns-1)*out.gap)/out.columns);
    out.cardHeight=out.cardWidth*9/16;
    return out;
}
Rect fitVideo(int width,int height,Rect view) {
    if(width<=0 || height<=0 || view.w<=0 || view.h<=0) return {};
    const double scale=std::min((double)view.w/width,(double)view.h/height);
    const int w=std::max(1,(int)floor(width*scale)),h=std::max(1,(int)floor(height*scale));
    return {view.x+(view.w-w)/2,view.y+(view.h-h)/2,w,h};
}
namespace {
void countCatalog(const Json& node,CatalogStats& stats,unsigned depth) {
    if(depth>64) return;
    if(node.get("videoId")) ++stats.videoIds;
    if(node.get("watchEndpoint")) ++stats.watchEndpoints;
    if(node.get("tileHeaderRenderer") || node.get("trackTileHeaderRenderer")) ++stats.tileHeaders;
    if(const auto* overlays=node.get("thumbnailOverlays"))
        for(const auto& overlay:overlays->children) if(overlay.get("tileMetadataRenderer")) ++stats.overlayMetadata;
    if(node.get("lockupViewModel")) ++stats.lockups;
    for(const auto& child:node.children) countCatalog(child,stats,depth+1);
}
}
CatalogStats catalogStats(const Json& response) {CatalogStats stats;countCatalog(response,stats,0);return stats;}
std::vector<Video> catalog(const Json& response) { std::vector<Video> result;videos(response,result,0);return result; }
PlayerInfo playerInfo(const Json& response) {
    PlayerInfo out;
    if(const auto* status=response.get("playabilityStatus")) {out.status=status->string("status");out.reason=status->string("reason");}
    if(const auto* details=response.get("videoDetails")) {if(const auto* flag=details->get("isLive")) out.live=flag->type==Json::Bool && flag->text=="true";out.title=details->string("title");out.videoId=details->string("videoId");const auto length=details->string("lengthSeconds");if(length.size()<=5 && length.find_first_not_of("0123456789")==std::string::npos && !length.empty()) {const auto seconds=strtoul(length.c_str(),nullptr,10);if(seconds<=43200) out.durationMs=(unsigned)seconds*1000;}}
    if(const auto* data=response.get("streamingData")) {
        out.hls=data->string("hlsManifestUrl");out.dash=data->string("dashManifestUrl");out.sabr=data->string("serverAbrStreamingUrl");
        for(const char* key:{"formats","adaptiveFormats"}) {
            if(const auto* list=data->get(key)) for(const auto& item:list->children) {
                Stream stream;
                stream.itag=item.integer("itag");stream.width=item.integer("width");stream.height=item.integer("height");
                stream.fps=item.integer("fps",30);stream.bitrate=item.integer("bitrate");stream.mime=item.string("mimeType");
                stream.url=item.string("url");stream.cipher=item.string("signatureCipher");
                if(stream.cipher.empty()) stream.cipher=item.string("cipher");
                if(const auto* track=item.get("audioTrack")) {
                    stream.audioId=track->string("id");stream.audioName=track->string("displayName");
                    stream.audioLanguage=stream.audioId.substr(0,stream.audioId.find('.'));
                    if(const auto* flag=track->get("audioIsDefault")) stream.audioDefault=flag->text=="true";
                    if(const auto* flag=track->get("isAutoDubbed")) stream.autoDubbed=flag->text=="true";
                }
                stream.adaptive=std::string(key)=="adaptiveFormats";
                stream.requiresSignature=!stream.cipher.empty() || stream.url.find("&n=")!=std::string::npos || stream.url.find("?n=")!=std::string::npos;
                out.streams.push_back(stream);
            }
        }
    }
    return out;
}
PlaybackChoice chooseStream(const PlayerInfo& info,DecoderCaps caps,int height) {
    if(info.status!="OK") return {PlaybackRequirement::Unplayable};
    if(!caps.avc || !caps.aac) return {PlaybackRequirement::DecoderUnverified};
    int best=-1;bool signatures=false,adaptive=false;
    const int maxHeight=std::min(caps.maxHeight,height>0?height:caps.maxHeight);
    for(size_t i=0;i<info.streams.size();++i) {
        const auto& stream=info.streams[i];
        if(stream.mime.find("avc1")==std::string::npos) continue;
        if(stream.requiresSignature) {signatures=true;continue;}
        if(stream.adaptive) {adaptive=adaptive || !stream.url.empty();continue;}
        if(stream.mime.find("mp4a")==std::string::npos || stream.url.compare(0,8,"https://")) continue;
        if(stream.width<=0 || stream.height<=0 || stream.height>maxHeight || stream.width>caps.maxWidth || stream.fps>caps.maxFps) continue;
        if(best<0 || stream.height>info.streams[best].height ||
           (stream.height==info.streams[best].height && stream.bitrate>info.streams[best].bitrate)) best=(int)i;
    }
    if(best>=0) return {PlaybackRequirement::DirectMuxedAvc,best};
    if(signatures) return {PlaybackRequirement::SignatureOrTokenResolver};
    if(adaptive || !info.dash.empty()) return {PlaybackRequirement::DashDemuxer};
    if(!info.sabr.empty()) return {PlaybackRequirement::SabrTransport};
    return {PlaybackRequirement::NoCompatibleStream};
}
Request searchRequest(const std::string& query,const std::string& client,const std::string& version) {
    return request("search","\"query\":"+quoteJson(query),client,version);
}
namespace {
struct AnonymousProfile {const char *tag,*name,*version,*number,*agent,*model,*os;};
// Pinned upstream Constants.kt, DefaultHeaders.kt and VideoInfoService.CLIENT_LIST.
const AnonymousProfile anonymousProfiles[]={
    {"TV","TVHTML5","7.20260901.15.00","7",
     "Mozilla/5.0 (Linux armeabi-v7a; Android 7.1.2; Fire OS 6.0) Cobalt/22.lts.3.306369-gold (unlike Gecko) v8/8.8.278.8-jit gles Starboard/13, Amazon_ATV_mediatek8695_2019/NS6294 (Amazon, AFTMM, Wireless) com.amazon.firetv.youtube/22.3.r2.v66.0","",""},
    {"VISIONOS","VISIONOS","1.02","101",
     "Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15","RealityDevice17,1","26.5.23O471"},
    {"TV_DOWNGRADED","TVHTML5","5.20260901","7",
     "Mozilla/5.0 (DirectFB; Linux x86_64) Cobalt/4.13031-qa (unlike Gecko) Starboard/1","",""}
};
}
const char* anonymousProfileName(unsigned index) {return index<AnonymousProfiles?anonymousProfiles[index].tag:"INVALID";}
Request anonymousPlayerRequest(const std::string& id,unsigned index,const std::string& visitor) {
    if(index>=AnonymousProfiles) return {};
    const auto& p=anonymousProfiles[index];
    Request out=playerRequest(id,p.name,p.version);
    out.userAgent=p.agent;out.clientNumber=p.number;
    // Use server-issued visitor context, never a fabricated token or account cookie.
    out.visitorData=visitor.size()<=8192?visitor:std::string();
    out.body="{\"context\":{\"client\":{\"clientName\":"+quoteJson(p.name)+",\"clientVersion\":"+quoteJson(p.version)+
        ",\"clientScreen\":\"WATCH\",\"userAgent\":"+quoteJson(p.agent)+",\"hl\":\"en\",\"gl\":\"US\",\"acceptLanguage\":\"en-US\",\"acceptRegion\":\"US\",\"utcOffsetMinutes\":0";
    if(*p.model) out.body+=",\"deviceModel\":"+quoteJson(p.model)+",\"osVersion\":"+quoteJson(p.os);
    if(!out.visitorData.empty()) out.body+=",\"visitorData\":"+quoteJson(out.visitorData);
    out.body+="}},\"videoId\":"+quoteJson(id)+"}";
    return out;
}
Request playerRequest(const std::string& id,const std::string& client,const std::string& version) {
    // This describes the endpoint contract; client capabilities, visitor/session
    // context and token/signature processing still belong in the service port.
    return request("player","\"videoId\":"+quoteJson(id),client,version);
}
}
