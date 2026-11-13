#pragma once
#include "shared/core.h"
#include "catalog_shelves.h"
#include <set>
namespace smarttube {
inline const Json* jsonPath(const Json& root,std::initializer_list<const char*> keys) {const Json* p=&root;for(auto* key:keys) {p=p->get(key);if(!p) return nullptr;}return p;}
inline std::string firstString(const Json& node,const char* key,unsigned depth=0) {if(depth>48) return {};auto s=node.string(key);if(!s.empty()) return s;for(const auto& child:node.children) {s=firstString(child,key,depth+1);if(!s.empty()) return s;}return {};}
inline std::string continuationIn(const Json& node) {auto value=firstString(node,"continuation");if(value.empty()) if(auto* command=node.get("continuationCommand")) value=command->string("token");if(value.empty()) for(const auto& child:node.children) {value=continuationIn(child);if(!value.empty()) break;}return value.size()<=16384?value:std::string();}
inline std::string videoDescription(const Json& root,unsigned depth=0) {
    if(depth>64) return {};
    if(auto* details=root.get("videoDetails")) {auto value=details->string("shortDescription");if(!value.empty()) return value;}
    for(const char* key:{"expandableVideoDescriptionBodyRenderer","videoDescriptionBodyRenderer","videoMetadataRenderer","videoSecondaryInfoRenderer"}) if(auto* renderer=root.get(key)) {
        for(const char* field:{"descriptionBodyText","attributedDescriptionBodyText","attributedDescription","description"}) {auto value=nodeText(renderer->get(field));if(!value.empty()) return value;}
    }
    for(const auto& child:root.children) {auto value=videoDescription(child,depth+1);if(!value.empty()) return value;}return {};
}
struct CaptionTrack {std::string label,language,url;};
struct CaptionCue {unsigned start=0,end=0;std::string text;};
struct PlayerFeatures {
    std::string title,channelId,channelName,channelIcon,commentsToken,chatToken,views,published,likes,dislikes,subscribers;int reaction=0;bool subscribed=false;std::vector<CaptionTrack> tracks;
    void player(const Json& root) {
        if(auto* details=root.get("videoDetails")) {title=details->string("title");channelId=details->string("channelId");channelName=details->string("author");auto count=details->string("viewCount");if(!count.empty()) views=count+" views";}
        if(auto* micro=jsonPath(root,{"microformat","playerMicroformatRenderer"})) {auto date=micro->string("publishDate");if(date.empty()) date=micro->string("uploadDate");if(!date.empty()) published=date;}
        if(auto* captions=jsonPath(root,{"captions","playerCaptionsTracklistRenderer","captionTracks"})) {
            tracks.clear();for(const auto& track:captions->children) {auto url=track.string("baseUrl");if(url.compare(0,8,"https://") || url.size()>16384) continue;tracks.push_back({nodeText(track.get("name")),track.string("languageCode"),url});}
        }
    }
    void apply(Video& item) const {
        if(!channelName.empty()) item.channel=channelName;if(!views.empty()) item.views=views;if(!published.empty()) item.published=published;
        if(!likes.empty()) item.likes=likes;if(!dislikes.empty()) item.dislikes=dislikes;if(!subscribers.empty()) item.subscribers=subscribers;
    }
    void next(const Json& root) {
        const auto visit=[&](const auto& self,const Json& node,unsigned depth)->void {
            if(depth>64) return;
            for(const char* key:{"videoPrimaryInfoRenderer","videoMetadataRenderer","slimVideoMetadataRenderer"}) if(auto* metadata=node.get(key)) {
                if(auto* count=metadata->get("viewCount")) {auto value=nodeText(count->get("videoViewCountRenderer")?jsonPath(*count,{"videoViewCountRenderer","viewCount"}):count);if(!value.empty()) views=value;}
                auto value=nodeText(metadata->get("viewCountText"));if(!value.empty()) views=value;
                for(const char* field:{"dateText","publishedTimeText","publishedTime","publishDate"}) {value=nodeText(metadata->get(field));if(value.empty()) if(auto* date=metadata->get(field)) value=firstString(*date,"label");if(!value.empty()) published=value;}
            }
            if(auto* count=node.get("likesCount")) {auto value=nodeText(count);if(!value.empty()) likes=value;}
            if(auto* count=node.get("dislikesCount")) {auto value=nodeText(count);if(!value.empty()) dislikes=value;}
            if(auto* button=node.get("likeButtonRenderer")) {auto value=nodeText(button->get("likeCountText"));if(value.empty()) if(auto* count=button->get("likeCount")) value=count->text;if(!value.empty()) likes=value;}
            if(auto* button=node.get("toggleButtonRenderer")) {auto icon=firstString(*button,"iconType"),count=nodeText(button->get("defaultText"));if(count.find_first_of("0123456789")!=std::string::npos) {if(icon=="LIKE") likes=count;if(icon=="DISLIKE") dislikes=count;}}
            if(auto* owner=node.get("videoOwnerRenderer")) {auto count=nodeText(owner->get("subscriberCountText"));if(!count.empty()) subscribers=count;auto title=nodeText(owner->get("title"));if(!title.empty()) channelName=title;auto id=firstString(*owner,"browseId");if(id.compare(0,2,"UC")==0) channelId=id;
                if(auto* thumbs=jsonPath(*owner,{"thumbnail","thumbnails"})) for(const auto& thumb:thumbs->children) {auto url=thumb.string("url");if(url.compare(0,8,"https://")==0) channelIcon=url;}
            }
            auto like=node.string("likeStatus");if(like=="LIKE") reaction=1;if(like=="DISLIKE") reaction=-1;
            if(auto* button=node.get("subscribeButtonRenderer")) if(auto* state=button->get("subscribed")) subscribed=state->text=="true";
            for(size_t i=0;i<node.children.size();++i) {auto key=i<node.keys.size()?node.keys[i]:std::string();
                if(key=="liveChatRenderer") {auto token=continuationIn(node.children[i]);if(!token.empty()) chatToken=token;}
                if(key=="engagementPanelSectionListRenderer" || key=="commentsEntryPointHeaderRenderer" || key=="commentSectionRenderer") {auto token=continuationIn(node.children[i]);auto target=firstString(node.children[i],"targetId");if(key!="engagementPanelSectionListRenderer" || target.find("comment")!=std::string::npos) if(!token.empty()) commentsToken=token;}
                self(self,node.children[i],depth+1);
            }
        };visit(visit,root,0);
    }
};
inline std::vector<CaptionCue> captionCues(const Json& root) {
    std::vector<CaptionCue> out;if(auto* events=root.get("events")) for(const auto& event:events->children) {
        int start=event.integer("tStartMs",-1),duration=event.integer("dDurationMs");if(start<0 || duration<=0) continue;std::string text;
        if(auto* segments=event.get("segs")) for(const auto& segment:segments->children) text+=segment.string("utf8");if(text.empty() || text.size()>4096) continue;
        out.push_back({(unsigned)start,(unsigned)((uint64_t)start+duration>UINT32_MAX?UINT32_MAX:start+duration),text});if(out.size()>=10000) break;
    }
    std::stable_sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.start<b.start;});return out;
}
inline std::string captionAt(const std::vector<CaptionCue>& cues,unsigned position) {
    auto cue=std::upper_bound(cues.begin(),cues.end(),position,[](unsigned p,const CaptionCue& c){return p<c.start;});if(cue==cues.begin()) return {};--cue;return position<cue->end?cue->text:std::string();
}
struct DiscussionLine {std::string author,text;};
inline std::vector<DiscussionLine> discussion(const Json& root,bool chat) {
    std::vector<DiscussionLine> out;
    const auto visit=[&](const auto& self,const Json& node,unsigned depth)->void {if(depth>64 || out.size()>=200) return;
        const Json* comment=node.get(chat?"liveChatTextMessageRenderer":"commentRenderer");
        if(comment) {auto author=nodeText(comment->get(chat?"authorName":"authorText")),text=nodeText(comment->get(chat?"message":"contentText"));if(!text.empty()) out.push_back({author.substr(0,160),text.substr(0,3000)});return;}
        if(!chat) if(auto* entity=node.get("commentEntityPayload")) {auto author=jsonPath(*entity,{"author","displayName"}),text=jsonPath(*entity,{"properties","content"});auto body=nodeText(text);if(!body.empty()) out.push_back({nodeText(author).substr(0,160),body.substr(0,3000)});return;}
        for(const auto& child:node.children) self(self,child,depth+1);
    };visit(visit,root,0);return out;
}
inline Request featureRequest(const std::string& endpoint,const std::string& fields,const std::string& authorization="") {
    Request request;request.url="https://www.youtube.com/youtubei/v1/"+endpoint;request.clientNumber="7";request.clientVersion="7.20260901.15.00";request.authorization=authorization;
    request.body=R"({"context":{"client":{"clientName":"TVHTML5","clientVersion":"7.20260901.15.00","tvAppInfo":{"appQuality":"TV_APP_QUALITY_LIMITED_ANIMATION","zylonLeftNav":true},"webpSupport":false,"hl":"en","gl":"US"}},)"+fields+"}";return request;
}
inline Request shortsRequest(const std::string& token="") {
    Request request;request.clientNumber="1";request.clientVersion="2.20260917.01.00";
    request.url="https://www.youtube.com/youtubei/v1/reel/"+std::string(token.empty()?"reel_item_watch":"reel_watch_sequence");
    request.body=R"({"context":{"client":{"clientName":"WEB","clientVersion":"2.20260917.01.00","hl":"en","gl":"US"}},)"+(token.empty()?std::string(R"("disablePlayerResponse":true,"inputType":"REEL_WATCH_INPUT_TYPE_SEEDLESS","params":"CA8%3D")"):"\"sequenceParams\":"+quoteJson(token))+"}";return request;
}
inline std::vector<Video> reelVideos(const Json& root) {
    std::vector<Video> out;std::set<std::string> seen;
    const auto add=[&](const std::string& id,const std::string& title) {if(id.size()!=11 || id.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos || !seen.insert(id).second) return;out.push_back({id,title.empty()?"Shorts video":title,"","https://i.ytimg.com/vi/"+id+"/hqdefault.jpg","SHORTS"});};
    if(auto* replacement=root.get("replacementEndpoint")) add(firstString(*replacement,"videoId"),"");
    const auto visit=[&](const auto& self,const Json& node,unsigned depth)->void {if(depth>64 || out.size()>=100) return;if(auto* reel=node.get("reelWatchEndpoint")) add(reel->string("videoId"),nodeText(node.get("title")));for(const auto& child:node.children) self(self,child,depth+1);};visit(visit,root,0);return out;
}
struct EditablePlaylist {std::string id,title;bool contains=false;};
inline std::vector<EditablePlaylist> editablePlaylists(const Json& root) {
    std::vector<EditablePlaylist> out;const auto visit=[&](const auto& self,const Json& node)->void {
        if(out.size()>=100) return;if(auto* item=node.get("playlistAddToOptionRenderer")) {auto id=item->string("playlistId"),title=nodeText(item->get("title"));if(!id.empty() && !title.empty()) out.push_back({id,title,item->string("containsSelectedVideos")=="ALL"});return;}for(const auto& child:node.children) self(self,child);
    };visit(visit,root);return out;
}
}
