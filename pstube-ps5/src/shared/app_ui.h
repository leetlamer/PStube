#pragma once
#include "app_state.h"
#include "renderer.h"
#include "../catalog_shelves.h"
#include <map>
#include <memory>
namespace smarttube {
struct Image {int width=0,height=0;std::vector<uint32_t> pixels;};
std::shared_ptr<Image> decodeThumbnail(const std::string& bytes,int targetWidth=640,int targetHeight=360);
class Canvas {
public:
    Canvas();~Canvas();
    bool loadFont(const char* path);
    void surface(cube::Surface value) {surface_=value;}
    void rect(int x,int y,int width,int height,uint32_t color);
    void text(const std::string& value,int x,int y,int size,uint32_t color,int width=1800,int lines=1);
    void image(const Image& value,Rect view);
    unsigned loadIcons();
    bool icon(const std::string& name,Rect view,uint32_t tint=0xffeef6ff);
    void marquee(const std::string& value,int x,int y,int size,uint32_t color,int width,double seconds);
    void accountQr(const std::string& url,int x,int y,int scale);
    void playIcon(int x,int y,int size,uint32_t color);
private:
    friend void drawPlayer(Canvas&,const Video&,const Image&,bool,bool,bool,uint64_t,unsigned,int,int,bool,bool,bool,EndMode);
    friend void drawPlaybackDebug(Canvas&,const std::vector<std::string>&);
    struct Impl;Impl* impl_;
    cube::Surface surface_{};
};
enum Menu {Home=0,Search,Subscriptions,Channels,AccountHistory,Playlists,Liked,Music,Gaming,Live,News,Shorts,Favorites,LocalHistory,Settings};
constexpr int MenuCount=15;
const char* menuLabel(int index);
struct BrowseView {
    int menu=0;bool sidebar=false;
    std::string heading="Home",subtitle,status,toast;
    std::vector<Video> videos;
    std::map<std::string,std::shared_ptr<Image>> thumbnails;
    const Library* library=nullptr;
    size_t selected=0;
    bool busy=false,linked=false,channelList=false;
    std::vector<Shelf> shelves;
    std::string accountName,description;double selectedSeconds=0;
    std::shared_ptr<Image> accountImage;
};
void drawBrowse(Canvas& canvas,const BrowseView& view);
void drawKeyboard(Canvas& canvas,const Keyboard& keyboard);
void drawAccountOptions(Canvas& canvas,int selected,const std::string& name,const std::string& status,int startup,unsigned saved);
void drawAccount(Canvas& canvas,const std::string& status,const std::string& code,const std::string& url);
void drawSettings(Canvas& canvas,const Library& library,int selected);
void drawPlayer(Canvas& canvas,const Video& video,const Image& frame,bool controls,bool paused,bool ended,
                uint64_t position,unsigned duration,int height,int control=-1,bool debug=false,bool settingsSidebar=false,bool live=false,EndMode endMode=EndMode::AutoNext);
void drawPlaybackDebug(Canvas& canvas,const std::vector<std::string>& lines);
void drawPlaybackSettings(Canvas& canvas,const Preferences& pending,int selected,int actualHeight,const std::string& available,bool debug,uint64_t testPosition=UINT64_MAX);
void drawQuality(Canvas& canvas,int selected,int actualHeight);
void drawLoading(Canvas& canvas,const Video& video,const std::string& status);
}

