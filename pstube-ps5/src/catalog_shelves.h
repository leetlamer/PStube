#pragma once
#include "shared/core.h"
#include <algorithm>
namespace smarttube {
struct Shelf {std::string title;size_t first=0,count=0,cursor=0;std::string continuation{};};
inline std::string nodeText(const Json* node) {
    if(!node) return {};if(node->type==Json::String) return node->text;std::string out=node->string("simpleText");if(out.empty()) out=node->string("content");
    if(auto* runs=node->get("runs")) for(const auto& run:runs->children) out+=run.string("text");return out;
}
inline std::string shelfTitle(const Json& shelf) {
    std::string title=nodeText(shelf.get("title"));
    const Json* header=shelf.get("headerRenderer");
    header=header?header->get("shelfHeaderRenderer"):nullptr;
    if(title.empty() && header) title=nodeText(header->get("title"));
    const Json* avatar=header?header->get("avatarLockup"):nullptr;
    avatar=avatar?avatar->get("avatarLockupRenderer"):nullptr;
    if(title.empty() && avatar) title=nodeText(avatar->get("title"));
    return title;
}
inline void shelfCatalog(const Json& root,std::vector<Video>& items,std::vector<Shelf>& shelves,bool fallback=true) {
    items.clear();shelves.clear();
    const auto visit=[&](const auto& self,const Json& node,unsigned depth)->void {
        if(depth>64 || shelves.size()>=40 || items.size()>=1000) return;
        for(const char* key:{"shelfRenderer","richShelfRenderer"}) if(auto* shelf=node.get(key)) {
            auto found=catalog(*shelf);if(!found.empty()) {std::string title=shelfTitle(*shelf);if(title.empty()) title="Videos";
                Shelf row{title,items.size(),0,0};for(auto& v:found) if(items.size()<1000) {items.push_back(std::move(v));++row.count;}row.continuation=catalogContinuation(*shelf);shelves.push_back(std::move(row));return;}
        }
        for(const auto& child:node.children) self(self,child,depth+1);
    };visit(visit,root,0);
    if(shelves.empty() && fallback) {items=catalog(root);if(!items.empty()) shelves.push_back({"Videos",0,items.size(),0});}
}
// A feed continuation adds category rows. Never consume a nested row's token here.
inline std::string feedContinuation(const Json& root,unsigned depth=0) {
    if(depth>64) return {};
    for(const char* key:{"nextContinuationData","reloadContinuationData"}) if(auto* next=root.get(key)) {auto token=next->string("continuation");if(token.size()<=16384 && !token.empty()) return token;}
    if(auto* command=root.get("continuationCommand")) {auto token=command->string("token");if(token.size()<=16384 && !token.empty()) return token;}
    for(size_t i=0;i<root.children.size();++i) {
        if(root.type==Json::Object && i<root.keys.size() && (root.keys[i]=="shelfRenderer" || root.keys[i]=="richShelfRenderer" || root.keys[i]=="horizontalListRenderer")) continue;
        auto token=feedContinuation(root.children[i],depth+1);if(!token.empty()) return token;
    }return {};
}
inline size_t appendShelfPage(const Json& root,std::vector<Video>& items,std::vector<Shelf>& shelves) {
    std::vector<Video> page;std::vector<Shelf> rows;shelfCatalog(root,page,rows,false);size_t added=0;
    for(auto& row:rows) {
        if(shelves.size()>=40 || items.size()>=1000) break;bool duplicate=false;
        for(const auto& old:shelves) if(old.title==row.title) {bool all=true;for(size_t i=row.first;i<row.first+row.count;++i) {bool found=false;for(size_t j=old.first;j<old.first+old.count;++j) if(items[j].id==page[i].id) found=true;if(!found) all=false;}if(all) duplicate=true;}
        if(duplicate) continue;Shelf next{row.title,items.size(),0,0,row.continuation};
        for(size_t i=row.first;i<row.first+row.count && items.size()<1000;++i) {items.push_back(std::move(page[i]));++next.count;}
        if(next.count) {shelves.push_back(std::move(next));++added;}
    }return added;
}
inline size_t appendShelfVideos(const Json& root,size_t row,std::vector<Video>& items,std::vector<Shelf>& shelves) {
    if(row>=shelves.size()) return 0;auto found=catalog(root);std::vector<Video> added;
    for(auto& item:found) {bool duplicate=false;for(size_t i=shelves[row].first;i<shelves[row].first+shelves[row].count;++i) if(items[i].id==item.id) duplicate=true;for(const auto& next:added) if(next.id==item.id) duplicate=true;if(!duplicate && items.size()+added.size()<1000) added.push_back(std::move(item));}
    size_t at=shelves[row].first+shelves[row].count;items.insert(items.begin()+at,added.begin(),added.end());shelves[row].count+=added.size();for(size_t i=row+1;i<shelves.size();++i) shelves[i].first+=added.size();return added.size();
}
inline size_t selectedShelf(const std::vector<Shelf>& shelves,size_t selected) {
    for(size_t i=0;i<shelves.size();++i) if(selected>=shelves[i].first && selected<shelves[i].first+shelves[i].count) return i;return 0;
}
inline size_t moveShelf(std::vector<Shelf>& shelves,size_t selected,int dx,int dy) {
    if(shelves.empty()) return selected;size_t row=selectedShelf(shelves,selected),column=selected-shelves[row].first;
    if(dx<0 && column) --column;if(dx>0 && column+1<shelves[row].count) ++column;
    shelves[row].cursor=column;
    if(dy<0 && row) {--row;column=std::min(shelves[row].cursor,shelves[row].count-1);}
    if(dy>0 && row+1<shelves.size()) {++row;column=std::min(shelves[row].cursor,shelves[row].count-1);}
    shelves[row].cursor=column;return shelves[row].first+column;
}
}
