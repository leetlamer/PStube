#include "app_ui.h"
#include "../generated/smarttube_icons.h"
#include "login_qr.h"
#include <algorithm>
#include <cmath>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 2048
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
namespace smarttube {
namespace {
constexpr uint32_t background=0xff000000,muted=0xff9db4cc,white=0xffeef6ff,red=0xff329dff;
struct Glyph {int width=0,height=0,left=0,top=0,advance=0;std::vector<uint8_t> alpha;};
uint32_t blend(uint32_t under,uint32_t color,unsigned alpha) {
    const auto channel=[&](unsigned shift) {return ((((under>>shift)&255)*(255-alpha)+((color>>shift)&255)*alpha+127)/255)<<shift;};
    return 0xff000000|channel(16)|channel(8)|channel(0);
}
unsigned codepoint(const std::string& text,size_t& p) {
    const unsigned ch=(uint8_t)text[p++];if(ch<128) return ch;
    unsigned count=0,value=0;
    if((ch&0xe0)==0xc0) {count=1;value=ch&31;}else if((ch&0xf0)==0xe0) {count=2;value=ch&15;}else if((ch&0xf8)==0xf0) {count=3;value=ch&7;}else return '?';
    if(text.size()-p<count) {p=text.size();return '?';}
    for(unsigned i=0;i<count;++i) {const unsigned next=(uint8_t)text[p++];if((next&0xc0)!=0x80) return '?';value=(value<<6)|(next&63);}
    if(value>0x10ffff || (value>=0xd800 && value<=0xdfff)) return '?';return value;
}
std::string timeText(uint64_t ms) {char b[32];snprintf(b,sizeof(b),"%u:%02u",(unsigned)(ms/60000),(unsigned)(ms/1000%60));return b;}
}
struct Canvas::Impl {
    stbtt_fontinfo font{};std::vector<uint8_t> bytes;std::map<uint64_t,Glyph> glyphs;bool loaded=false;
    struct TextRun {std::vector<unsigned> codepoints;std::vector<int> positions;int total=0;};std::map<std::pair<std::string,int>,TextRun> marqueeRuns;
    Image qrLayer;std::string qrUrl;std::map<std::string,Image> icons;
    Image playerTop,playerBottom;std::vector<std::string> playerKey;
    Image debugLayer;std::vector<std::string> debugLines;bool debugValid=false;
    Glyph& glyph(unsigned cp,int size) {
        if(!stbtt_FindGlyphIndex(&font,(int)cp)) cp='?';
        const uint64_t key=((uint64_t)size<<32)|cp;
        auto found=glyphs.find(key);if(found!=glyphs.end()) return found->second;
        if(glyphs.size()>=2048) glyphs.clear();
        Glyph value;const float scale=stbtt_ScaleForPixelHeight(&font,(float)size);int advance=0,bearing=0;
        stbtt_GetCodepointHMetrics(&font,(int)cp,&advance,&bearing);value.advance=std::max(1,(int)(advance*scale+0.5f));
        auto* bitmap=stbtt_GetCodepointBitmap(&font,scale,scale,(int)cp,&value.width,&value.height,&value.left,&value.top);
        if(bitmap && value.width>0 && value.height>0) value.alpha.assign(bitmap,bitmap+(size_t)value.width*value.height);
        stbtt_FreeBitmap(bitmap,nullptr);return glyphs.emplace(key,std::move(value)).first->second;
    }
};
Canvas::Canvas():impl_(new Impl) {}
Canvas::~Canvas() {delete impl_;}
bool Canvas::loadFont(const char* path) {
    impl_->debugValid=false;impl_->playerKey.clear();impl_->glyphs.clear();impl_->marqueeRuns.clear();
    FILE* f=fopen(path,"rb");if(!f) return false;char bytes[4096];size_t count;impl_->bytes.clear();
    while((count=fread(bytes,1,sizeof(bytes),f)) && impl_->bytes.size()+count<=1024*1024) impl_->bytes.insert(impl_->bytes.end(),bytes,bytes+count);
    const bool valid=feof(f) && !ferror(f);fclose(f);
    impl_->loaded=valid && !impl_->bytes.empty() && stbtt_InitFont(&impl_->font,impl_->bytes.data(),0);return impl_->loaded;
}
void Canvas::rect(int x,int y,int w,int h,uint32_t color) {
    x+=originX_;y+=originY_;
    const int right=std::min(std::min(surface_.width,clip_.x+clip_.w),x+w),bottom=std::min(std::min(surface_.height,clip_.y+clip_.h),y+h);
    if(right<=std::max(std::max(0,clip_.x),x) || bottom<=std::max(std::max(0,clip_.y),y)) return;
    for(int row=std::max(std::max(0,clip_.y),y);row<bottom;++row) std::fill(surface_.pixels+(size_t)row*surface_.pitch+std::max(std::max(0,clip_.x),x),surface_.pixels+(size_t)row*surface_.pitch+right,color);
}
void Canvas::roundedRect(Rect v,int radius,uint32_t color) {
    int r=std::max(0,std::min(radius,std::min(v.w,v.h)/2));
    for(int y=0;y<v.h;++y) {int d=y<r?r-1-y:y>=v.h-r?y-(v.h-r):0;int inset=d?(int)std::ceil(r-std::sqrt((double)r*r-(double)d*d)):0;rect(v.x+inset,v.y+y,v.w-2*inset,1,color);}
}
void Canvas::roundImageTop(const Image& value,Rect v,int radius,uint32_t outside) {
    image(value,v);int r=std::max(0,std::min(radius,std::min(v.w,v.h)/2));
    for(int y=0;y<r;++y) {int d=r-1-y;int inset=(int)std::ceil(r-std::sqrt((double)r*r-(double)d*d));rect(v.x,v.y+y,inset,1,outside);rect(v.x+v.w-inset,v.y+y,inset,1,outside);}
}
void Canvas::text(const std::string& value,int x,int y,int size,uint32_t color,int width,int lines) {
    if(!impl_->loaded || size<8 || size>80 || lines<1) return;
    x+=originX_;y+=originY_;const int start=x,right=x+width;int line=0;
    for(size_t p=0;p<value.size();) {
        const auto cp=codepoint(value,p);auto& glyph=impl_->glyph(cp,size);
        if(cp=='\n' || x+glyph.advance>right) {if(++line>=lines) break;x=start;y+=size+7;if(cp=='\n') continue;}
        const int top=y+size+glyph.top;
        for(int row=0;row<glyph.height;++row) for(int col=0;col<glyph.width;++col) {
            const int xx=x+glyph.left+col,yy=top+row;
            if(xx<start || xx>=right || xx<std::max(0,clip_.x) || xx>=std::min(surface_.width,clip_.x+clip_.w) || yy<std::max(0,clip_.y) || yy>=std::min(surface_.height,clip_.y+clip_.h)) continue;
            const auto a=glyph.alpha[(size_t)row*glyph.width+col];if(!a) continue;
            auto& pixel=surface_.pixels[(size_t)yy*surface_.pitch+xx];pixel=blend(pixel,color,a);
        }
        x+=glyph.advance;
    }
}
unsigned Canvas::loadIcons() {
    const char* names[]={"icon_home","icon_subscriptions","icon_channels","icon_history","icon_playlist","icon_music","icon_gaming","icon_live","icon_news","icon_shorts","icon_settings","action_search","action_channel","action_chat","action_playlist_add","action_subscribe","action_video_stats","action_screen_off","settings_dearrow","lb_ic_play","lb_ic_pause","lb_ic_skip_next","lb_ic_skip_previous","lb_ic_thumb_up","lb_ic_thumb_down"};
    impl_->icons.clear();for(auto name:names) {const auto* asset=embeddedIcon(name);if(!asset) continue;int w=0,h=0,n=0;if(!stbi_info_from_memory(asset->data,(int)asset->size,&w,&h,&n) || w<1 || h<1 || w>512 || h>512) continue;auto* data=stbi_load_from_memory(asset->data,(int)asset->size,&w,&h,&n,4);if(!data) continue;
        if(w>0 && h>0 && w<=512 && h<=512) {Image image;image.width=image.height=64;image.pixels.resize(4096);for(int y=0;y<64;++y) for(int x=0;x<64;++x) {auto* pixel=data+((size_t)(y*h/64)*w+x*w/64)*4;image.pixels[y*64+x]=((uint32_t)pixel[3]<<24)|((uint32_t)pixel[0]<<16)|((uint32_t)pixel[1]<<8)|pixel[2];}impl_->icons.emplace(name,std::move(image));}stbi_image_free(data);
    }return (unsigned)impl_->icons.size();
}
bool Canvas::icon(const std::string& name,Rect view,uint32_t tint) {
    auto found=impl_->icons.find(name);if(found==impl_->icons.end() || view.w<=0 || view.h<=0) return false;auto& image=found->second;view.x+=originX_;view.y+=originY_;
    for(int y=std::max(0,view.y);y<std::min(surface_.height,view.y+view.h);++y) for(int x=std::max(0,view.x);x<std::min(surface_.width,view.x+view.w);++x) {auto pixel=image.pixels[(size_t)((y-view.y)*image.height/view.h)*image.width+(x-view.x)*image.width/view.w];unsigned alpha=pixel>>24;if(alpha) {auto& dst=surface_.pixels[(size_t)y*surface_.pitch+x];dst=blend(dst,tint,alpha);}}return true;
}
void Canvas::marquee(const std::string& value,int x,int y,int size,uint32_t color,int width,double seconds) {
    if(!impl_->loaded || size<8 || size>80 || width<=0) return;
    auto key=std::make_pair(value,size);auto found=impl_->marqueeRuns.find(key);
    if(found==impl_->marqueeRuns.end()) {if(impl_->marqueeRuns.size()>=8) impl_->marqueeRuns.clear();Impl::TextRun run;for(size_t p=0;p<value.size();) {auto cp=codepoint(value,p);if(cp=='\n' || cp=='\r') cp=' ';run.positions.push_back(run.total);run.codepoints.push_back(cp);run.total+=impl_->glyph(cp,size).advance;}found=impl_->marqueeRuns.emplace(std::move(key),std::move(run)).first;}
    auto& run=found->second;int total=run.total;
    if(total<=width) {std::string one=value;std::replace(one.begin(),one.end(),'\n',' ');std::replace(one.begin(),one.end(),'\r',' ');text(one,x,y,size,color,width);return;}
    x+=originX_;y+=originY_;double delay=1.2,travel=(total+width+90)/52.0;double phase=fmod(std::max(0.0,seconds),delay+travel);int shift=phase<=delay?0:(int)((phase-delay)*52);
    auto begin=std::upper_bound(run.positions.begin(),run.positions.end(),shift);size_t first=begin==run.positions.begin()?0:(size_t)(begin-run.positions.begin()-1);
    for(size_t i=first;i<run.codepoints.size();++i) {int cursor=x+run.positions[i]-shift;auto& glyph=impl_->glyph(run.codepoints[i],size);if(cursor>=x+width) break;int top=y+size+glyph.top;
        if(cursor+glyph.advance>=x) for(int yy=0;yy<glyph.height;++yy) for(int xx=0;xx<glyph.width;++xx) {int px=cursor+glyph.left+xx,py=top+yy;if(px<x || px>=x+width || px<std::max(0,clip_.x) || px>=std::min(surface_.width,clip_.x+clip_.w) || py<std::max(0,clip_.y) || py>=std::min(surface_.height,clip_.y+clip_.h)) continue;auto a=glyph.alpha[(size_t)yy*glyph.width+xx];if(a) {auto& dst=surface_.pixels[(size_t)py*surface_.pitch+px];dst=blend(dst,color,a);}}
    }
}

void Canvas::image(const Image& value,Rect view) {
    if(value.width<1 || value.height<1 || value.pixels.size()!=(size_t)value.width*value.height) {rect(view.x,view.y,view.w,view.h,0xff000000);return;}
    auto fit=fitVideo(value.width,value.height,view);
    if(fit.x!=view.x || fit.y!=view.y || fit.w!=view.w || fit.h!=view.h) rect(view.x,view.y,view.w,view.h,0xff000000);
    fit.x+=originX_;fit.y+=originY_;
    const int left=std::max(std::max(0,clip_.x),fit.x),right=std::min(std::min(surface_.width,clip_.x+clip_.w),fit.x+fit.w);
    const int top=std::max(std::max(0,clip_.y),fit.y),bottom=std::min(std::min(surface_.height,clip_.y+clip_.h),fit.y+fit.h);
    if(left>=right || top>=bottom || right-left>1920) return;
    // Full-width unscaled playback surfaces are contiguous. Copy once instead
    // of issuing a copy and row-address calculation for every scanline.
    if(fit.w==value.width && fit.h==value.height && left==fit.x && right-left==value.width && surface_.pitch==value.width) {
        memcpy(surface_.pixels+(size_t)top*surface_.pitch+left,
               value.pixels.data()+(size_t)(top-fit.y)*value.width,
               (size_t)(bottom-top)*value.width*sizeof(uint32_t));
        return;
    }
    int sourceX[1920];for(int x=left;x<right;++x) sourceX[x-left]=(x-fit.x)*value.width/fit.w;
    for(int y=top;y<bottom;++y) {
        const auto* source=value.pixels.data()+(size_t)((y-fit.y)*value.height/fit.h)*value.width;
        auto* target=surface_.pixels+(size_t)y*surface_.pitch+left;
        if(fit.w==value.width) memcpy(target,source+left-fit.x,(size_t)(right-left)*sizeof(uint32_t));
        else for(int x=0;x<right-left;++x) target[x]=source[sourceX[x]];
    }
}
void Canvas::playIcon(int x,int y,int size,uint32_t color) {for(int col=0;col<size;++col) rect(x+col,y+col/2,1,size-col,color);}
std::shared_ptr<Image> decodeThumbnail(const std::string& bytes,int targetWidth,int targetHeight) {
    if(targetWidth<1 || targetWidth>640 || targetHeight<1 || targetHeight>640 || (size_t)targetWidth*targetHeight>640u*360u) return {};
    if(bytes.empty() || bytes.size()>512*1024) return {};
    int width=0,height=0,channels=0;
    if(!stbi_info_from_memory((const stbi_uc*)bytes.data(),(int)bytes.size(),&width,&height,&channels) || width<1 || height<1 || width>1280 || height>1920) return {};
    auto* decoded=stbi_load_from_memory((const stbi_uc*)bytes.data(),(int)bytes.size(),&width,&height,&channels,3);if(!decoded) return {};
    auto out=std::make_shared<Image>();out->width=targetWidth;out->height=targetHeight;out->pixels.resize(targetWidth*targetHeight,0xff000000);
    const auto fit=fitVideo(width,height,{0,0,targetWidth,targetHeight});
    for(int y=0;y<fit.h;++y) for(int x=0;x<fit.w;++x) {
        const size_t p=((size_t)(y*height/fit.h)*width+x*width/fit.w)*3;
        out->pixels[(size_t)(fit.y+y)*targetWidth+fit.x+x]=0xff000000u|((uint32_t)decoded[p]<<16)|((uint32_t)decoded[p+1]<<8)|decoded[p+2];
    }
    stbi_image_free(decoded);return out;
}
void Canvas::accountQr(const std::string& url,int x,int y,int scale) {
    if(impl_->qrUrl!=url) {
        impl_->qrUrl=url;impl_->qrLayer={};const LoginQr qr(url);
        if(qr.valid) {auto& layer=impl_->qrLayer;layer.width=layer.height=LoginQr::size+8;layer.pixels.assign(layer.width*layer.height,0xffffffff);
            for(int yy=0;yy<LoginQr::size;++yy) for(int xx=0;xx<LoginQr::size;++xx) if(qr.modules[yy*LoginQr::size+xx]) layer.pixels[(yy+4)*layer.width+xx+4]=0xff000000;}
    }
    if(impl_->qrLayer.width) image(impl_->qrLayer,{x,y,impl_->qrLayer.width*scale,impl_->qrLayer.height*scale});
}
const char* menuLabel(int index) {
    static const char* labels[]={"Home","Search","Subscriptions","Channels","Your history","Playlists","Liked videos","Music","Gaming","Live videos","News","Shorts","Favorites","Local history","Settings"};return index>=0 && index<MenuCount?labels[index]:"Home";
}
void drawBrowse(Canvas& c,const BrowseView& v) {
    c.origin(0);c.clip({0,0,1920,1080});c.rect(0,0,1920,1080,background);
    double expanded=v.sidebarExpansion<0?(v.sidebar?1:0):std::max(0.0,std::min(1.0,v.sidebarExpansion));
    int shift=(int)std::lround(-288*(1-expanded));c.origin((int)std::lround(-300*(1-expanded)));
    c.rect(0,0,300,1080,0xff101010);c.rect(28,24,246,120,v.sidebar && v.menu==-1?red:0xff202020);
    if(v.linked && v.accountImage) c.image(*v.accountImage,{40,39,88,88});else {c.rect(40,39,88,88,0xff265779);c.text("G",62,57,43,white,60);}
    c.text(v.linked?(v.accountName.empty()?"Account":v.accountName):"login",140,45,22,white,126,2);
    if(v.linked) c.text("Accounts",140,102,17,muted,125);c.text("PStube",48,151,32,white,220);
    const char* menuIcons[]={"icon_home","action_search","icon_subscriptions","icon_channels","icon_history","icon_playlist","lb_ic_thumb_up","icon_music","icon_gaming","icon_live","icon_news","icon_shorts","icon_playlist","icon_history","icon_settings"};
    for(int i=0;i<MenuCount;++i) {int y=209+i*46;if(v.menu==i) {c.rect(30,y-5,246,43,v.sidebar?0xffeeeeee:0xff343434);c.rect(30,y-5,5,43,red);}c.icon(menuIcons[i],{47,y+8,26,26},v.menu==i && v.sidebar?0xff161616:white);c.text(menuLabel(i),84,y+4,24,v.menu==i && v.sidebar?0xff161616:(v.menu==i?white:muted),187);}
    c.text("PStube PS5 · Beta 0.19",48,970,19,muted,225);c.text("OPTIONS  Exit",48,1006,19,muted,225);
    c.origin(shift);c.text(v.heading,354,65,42,white,1400);c.text(v.subtitle,356,123,23,muted,1480);
    if(v.linked) c.text((v.accountName.empty()?"YouTube account":v.accountName)+" logged in",1320-shift,64,24,white,490,2);
    const int clipLeft=320+shift;c.clip({clipLeft,165,1900-clipLeft,760});
    auto card=[&](size_t index,int x,int y,bool portrait) {
        if(index>=v.videos.size()) return;const auto& item=v.videos[index];bool focused=index==v.selected && !v.sidebar;
        int w=portrait?300:360+(int)std::lround(72*(1-expanded)),ih=portrait?534:w*9/16,h=ih+95;uint32_t border=focused?red:background;
        c.roundedRect({x-4,y-4,w+8,h+8},22,border);c.roundedRect({x,y,w,h},18,0xff171717);
        auto image=v.thumbnails.find(item.id);if(image!=v.thumbnails.end() && image->second) c.roundImageTop(*image->second,{x,y,w,ih},18,border);
        else {c.roundedRect({x,y,w,ih},18,0xff202020);c.playIcon(x+w/2-18,y+ih/2-18,36,muted);}
        if(focused) c.marquee(item.title,x+14,y+ih+6,21,white,w-28,v.selectedSeconds);else c.text(item.title,x+14,y+ih+6,21,white,w-28);
        c.text(item.channel+(item.subscribers.empty()?"":" · "+item.subscribers),x+14,y+ih+32,17,muted,w-28);
        c.text(item.views+(item.published.empty()?"":(item.views.empty()?"":" · ")+item.published),x+14,y+ih+53,16,muted,w-28);
        c.icon("lb_ic_thumb_up",{x+14,y+ih+76,16,16},muted);c.text(item.likes.empty()?"—":item.likes,x+36,y+ih+73,16,muted,w/3);
        c.icon("lb_ic_thumb_down",{x+w/2,y+ih+76,16,16},muted);c.text(item.dislikes.empty()?"—":item.dislikes,x+w/2+22,y+ih+73,16,muted,w/3);
        if(!item.badge.empty()) {c.roundedRect({x+w-88,y+ih-29,80,23},5,0xff171717);c.text(item.badge,x+w-82,y+ih-28,16,white,74);}
    };
    if(v.channelList) {
        double offset=v.verticalOffset<0?(double)resultFirst(v.selected,true):v.verticalOffset;
        int first=std::max(0,(int)std::floor(offset)-1),end=std::min((int)v.videos.size(),(int)std::floor(offset)+8);
        for(int i=first;i<end;++i) {int y=190+(int)std::lround((i-offset)*120);bool focused=(size_t)i==v.selected && !v.sidebar;c.roundedRect({350,y,1490,108},12,focused?red:0xff171717);auto image=v.thumbnails.find(v.videos[i].id);if(image!=v.thumbnails.end() && image->second) c.image(*image->second,{370,y+10,160,88});c.text(v.videos[i].title,566,y+14,28,white,1020,2);c.text("Open channel videos",568,y+66,20,muted,950);}
    } else if(!v.shelves.empty()) {
        size_t active=selectedShelf(v.shelves,v.selected);double offset=v.verticalOffset<0?(double)(active?active-1:0):v.verticalOffset;
        int first=std::max(0,(int)std::floor(offset)-1),end=std::min((int)v.shelves.size(),(int)std::floor(offset)+4);
        for(int r=first;r<end;++r) {const auto& shelf=v.shelves[r];int y=170+(int)std::lround((r-offset)*370);c.text(shelf.title,354,y,26,white,1330);size_t cursor=(size_t)r==active?v.selected-shelf.first:shelf.cursor;
            double horizontal=(size_t)r<v.shelfOffsets.size()?v.shelfOffsets[r]:carouselTarget(cursor,shelf.count);
            int begin=std::max(0,(int)std::floor(horizontal)-1),last=std::min((int)shelf.count,(int)std::floor(horizontal)+6);
            for(int col=begin;col<last;++col) card(shelf.first+(size_t)col,366+(int)std::lround((col-horizontal)*(376+(int)std::lround(72*(1-expanded)))),y+34,false);
            c.text(std::to_string(cursor+1)+" / "+std::to_string(shelf.count),1725-shift,y,17,muted,130);
        }
    } else {
        int cols=v.shorts?5:ResultColumns,spacing=v.shorts?318:376+(int)std::lround(72*(1-expanded)),rowHeight=v.shorts?680:370;
        double target=v.shorts?(double)(v.selected/cols):(double)(resultFirst(v.selected)/cols);double offset=v.verticalOffset<0?target:v.verticalOffset;
        int first=std::max(0,(int)std::floor(offset)-1),end=std::min((int)((v.videos.size()+cols-1)/cols),(int)std::floor(offset)+(v.shorts?3:4));
        for(int row=first;row<end;++row) for(int col=0;col<cols;++col) card((size_t)row*cols+col,354+col*spacing,190+(int)std::lround((row-offset)*rowHeight),v.shorts);
    }
    c.clip({0,0,1920,1080});
    if(v.videos.empty()) {c.text(v.busy?"Loading videos...":"No videos here yet",440,330,36,white,1100);c.text("Press TRIANGLE to search or LEFT for the menu.",440,400,25,muted,1100,2);}
    if(!v.videos.empty()) c.text(std::to_string(v.selected+1)+" / "+std::to_string(v.videos.size()),1700-shift,133,22,muted,150);
    c.rect(340,944,1520-shift,2,0xff333333);
    if(!v.sidebar && !v.channelList && v.selected<v.videos.size()) {c.marquee(v.videos[v.selected].title,356,953,24,white,1480-shift,v.selectedSeconds);c.marquee(v.description.empty()?v.status:v.description,356,988,21,muted,1480-shift,v.selectedSeconds);}
    else c.text(v.toast.empty()?v.status:v.toast,356,967,23,v.toast.empty()?muted:white,1480-shift,2);
    c.text("D-pad  Browse     CROSS  Open     R1  More     TRIANGLE  Search     LEFT / CIRCLE  Menu",356,1036,18,muted,1480-shift);c.origin(0);
}
void drawKeyboard(Canvas& c,const Keyboard& k) {
    c.rect(310,170,1570,825,0xff102941);c.text("Search YouTube",390,210,38,white,1330);
    c.rect(390,280,1380,74,0xff353535);c.text(k.value+"|",412,294,34,white,1335);
    for(int row=0;row<5;++row) for(int col=0;col<(row==4?4:10);++col) {
        const int w=row==4?330:123,x=390+col*(row==4?350:140),y=397+row*96;
        const bool active=k.row==row && k.column==col;c.rect(x,y,w,75,active?white:0xff393939);
        c.text(k.label(row,col),x+(row==4?28:46),y+18,31,active?0xff141414:white,w-20);
    }
    c.text("R1  Search     L1  Delete     SQUARE  Upper/lower case     CIRCLE  Cancel",390,913,23,muted,1400);
}
void drawAccountOptions(Canvas& c,int selected,const std::string& name,const std::string& status,int startup,unsigned saved) {
    c.rect(310,170,1610,830,0xff102941);c.text("Accounts",390,205,40,white,1390);
    c.text("Active: "+(name.empty()?std::string("Guest"):name)+"  ·  "+std::to_string(saved)+" saved",390,274,27,white,1400);
    c.text("Startup: "+(startup<0?std::string("Last selected account"):startup==0?std::string("Guest"):"Saved account "+std::to_string(startup)),390,320,24,muted,1400);
    const char* rows[]={"Add / login with another account","Rotate to next saved account","Use Guest (keep saved accounts)","Set current account as startup default","Restore last selected account at startup","Remove current saved account"};
    for(int i=0;i<6;++i) {const int y=382+i*65;c.rect(382,y,1410,57,i==selected?red:0xff19334e);c.text(rows[i],407,y+12,26,white,1360);}
    c.text(status,390,818,23,muted,1380,2);c.text("UP / DOWN  Choose     CROSS  Select     CIRCLE  Back",390,918,24,muted,1400);
}
void drawAccount(Canvas& c,const std::string& status,const std::string& code,const std::string& url) {
    c.rect(310,170,1610,830,0xff102941);c.text("Google / YouTube login",390,210,40,white,1400);
    c.text(status,390,285,27,white,1350,2);
    if(!code.empty()) {
        c.text("1. Scan with your phone",390,395,31,white,780);
        c.text("2. Enter this code and approve sign-in",390,455,28,muted,780,2);
        c.text(code,390,555,56,white,800);c.text(url,390,654,23,muted,800,2);
        c.accountQr(url,1330,392,9);
    } else c.text("Link this console securely with your Google account.",390,400,30,muted,1350,2);
    c.text("CROSS  New sign-in code     CIRCLE  Back to accounts",390,880,25,muted,1350,2);
}
void drawSettings(Canvas& c,const Library& library,int selected) {
    c.rect(310,170,1610,780,background);const auto& p=library.preferences;
    const std::string rows[]={"Default quality limit: "+qualityLabel(p.height),p.seconds?"Playback limit: "+std::to_string(p.seconds)+" seconds":"Playback: Full video","At video end: "+std::string(endModeLabel(p.endMode)),"Clear local playback history","YouTube account","Frame rate: up to "+std::to_string(p.maxFps)+" fps","Audio: "+std::string(p.audioLanguage=="en"?"English":"Original"),"Video decoder: "+std::string(p.softwareCodecs?(p.preferAv1?"AV1 first (experimental)":"VP9 first (experimental)"):"Native AVC"),"CPU scaler: "+std::to_string(p.scalerThreads)+" threads"};
    for(int i=0;i<9;++i) {c.rect(370,202+i*59,1430,53,selected==i?white:0xff292929);c.text(rows[i],400,212+i*59,27,selected==i?0xff151515:white,1370);}
    c.text("LEFT / RIGHT or CROSS changes the selected setting.",390,749,25,muted,1360);
    c.text("30 fps is recommended. 60 fps is experimental. Display output is 1080p.",390,802,24,muted,1330,2);
    c.text("Supported quality target: up to 1080p. 1440p / 2160p are unsupported per-video experiments.",390,872,23,muted,1330,2);
}
void drawPlayer(Canvas& c,const Video& video,const Image& frame,bool controls,bool paused,bool ended,uint64_t position,unsigned duration,int height,int control,bool debug,bool settingsSidebar,bool live,EndMode endMode) {
    if(settingsSidebar) {
        c.rect(0,0,1920,1080,0xff101010);c.image(frame,{0,154,1160,681});
        c.text(video.title,40,32,30,white,1060,2);
        c.text((live?"LIVE":timeText(position)+" / "+timeText(duration))+"    "+std::to_string(height)+"p",40,882,26,white,1060);
        c.text(paused?"Paused":"Video continues while settings are open",40,932,23,muted,1060);
        return;
    }
    c.image(frame,{0,0,1920,1080});
    if(!controls && !ended) return;
    auto& cache=*c.impl_;
    const std::vector<std::string> key={video.title,std::to_string(position/1000),std::to_string(duration),std::to_string(height),std::to_string(control),std::to_string(paused),std::to_string(ended),std::to_string(debug),std::to_string(live),std::to_string(static_cast<int>(endMode))};
    if(cache.playerKey!=key) {
        const auto target=c.surface_;
        auto& upper=cache.playerTop;upper.width=1920;upper.height=122;upper.pixels.resize(1920*122);
        auto& lower=cache.playerBottom;lower.width=1920;lower.height=245;lower.pixels.resize(1920*245);
        c.surface({upper.pixels.data(),1920,122,1920});
    c.rect(0,0,1920,122,0xff0c2036);c.text(video.title,65,28,35,white,1780,2);
        c.surface({lower.pixels.data(),1920,245,1920});
        const auto rect=[&](int x,int y,int w,int h,uint32_t color){c.rect(x,y-835,w,h,color);};
        const auto text=[&](const std::string& value,int x,int y,int size,uint32_t color,int width=1800,int lines=1){c.text(value,x,y-835,size,color,width,lines);};
        const auto playIcon=[&](int x,int y,int size,uint32_t color){c.playIcon(x,y-835,size,color);};
    rect(0,835,1920,245,0xff0c2036);
    const char* buttons[]={ended?"Replay":paused?"Play":"Pause","Previous","Next","-10 seconds","+10 seconds","Settings",debug?"Stats: On":"Stats: Off",endModeLabel(endMode)};
    for(int i=0;i<8;++i) {
        const int x=66+i*225;const auto ink=control==i?0xff081321:white;
        rect(x,846,215,66,control==i?red:0xff19334e);
        if(i==0 && !paused && !ended) {rect(x+18,863,7,30,ink);rect(x+32,863,7,30,ink);}
        else if(i==0) playIcon(x+18,862,32,ink);
        else if(i==1) {rect(x+16,863,5,30,ink);for(int n=0;n<31;++n) {const int half=15-abs(n-15);rect(x+39-half,863+n,half+1,1,ink);}}
        else if(i==2) {playIcon(x+16,862,32,ink);rect(x+48,863,5,30,ink);}
        else if(i==3 || i==4) {text(i==3?"-10":"+10",x+11,865,24,ink,55);}
        else if(i==5) {for(int n=0;n<3;++n) {rect(x+15,862+n*12,32,3,ink);rect(x+21+n*6,858+n*12,5,11,ink);}}
        else if(i==6) {text("i",x+25,861,29,ink,30);}
        else if(endMode==EndMode::Stop) rect(x+19,866,24,24,ink);
        else if(endMode==EndMode::Loop) {rect(x+18,866,26,3,ink);playIcon(x+39,860,16,ink);rect(x+22,888,26,3,ink);for(int n=0;n<15;++n) {const int half=7-abs(n-7);rect(x+23-half,882+n,half+1,1,ink);}}
        else {playIcon(x+16,866,25,ink);rect(x+44,866,4,25,ink);}
        text(buttons[i],x+60,865,20,ink,148);
    }
    text(std::to_string(height)+(live?"p · LIVE HLS":"p · buffered stream"),1330,944,22,muted,520);
    rect(66,921,1788,6,0xff5a5a5a);const int progress=duration?std::min(1788,(int)(std::min((uint64_t)duration,position)*1788/duration)):0;rect(66,921,progress,6,red);
    text(live?"LIVE · "+timeText(position):timeText(position)+" / "+timeText(duration),65,944,25,white,1600);
    text("UP  Controls     CROSS  Pause / select     LEFT / RIGHT  Seek / choose     L1 / R1  Previous / next     TRIANGLE  Settings     CIRCLE  Back",65,1014,23,muted,1790);
        c.surface(target);cache.playerKey=key;
    }
    c.image(cache.playerTop,{0,0,1920,122});c.image(cache.playerBottom,{0,835,1920,245});
    // Keep the progress fill smooth without rerasterizing text each frame.
    c.rect(66,921,1788,6,0xff5a5a5a);const int progress=duration?std::min(1788,(int)(std::min((uint64_t)duration,position)*1788/duration)):0;c.rect(66,921,progress,6,red);

}
void drawPlaybackDebug(Canvas& c,const std::vector<std::string>& lines) {
    const std::vector<std::string> visible(lines.begin(),lines.begin()+std::min(lines.size(),size_t(4)));
    auto& cache=*c.impl_;
    if(!cache.debugValid || cache.debugLines!=visible) {
        auto& layer=cache.debugLayer;
        layer.width=1920;layer.height=lines.size()>3?157:122;
        layer.pixels.assign((size_t)layer.width*layer.height,0xff0c2036);
        const auto target=c.surface_;
        c.surface({layer.pixels.data(),layer.width,layer.height,layer.width});
        for(size_t i=0;i<visible.size();++i) c.text(visible[i],32,10+(int)i*35,24,white,1856);
        c.surface(target);
        cache.debugLines=visible;cache.debugValid=true;
    }
    // The complete banner has an opaque background. Reusing it avoids glyph
    // read-modify-write operations on uncached/write-combined scanout memory.
    c.image(cache.debugLayer,{0,0,1920,cache.debugLayer.height});
}

void drawPlaybackSettings(Canvas& c,const Preferences& p,int selected,int actualHeight,const std::string& available,bool debug,uint64_t) {
    c.rect(1160,140,760,940,0xff102941);
    c.text("Playback settings",1200,168,34,white,675);
    c.text("Playing "+std::to_string(actualHeight)+"p",1200,218,23,muted,675);
    c.text("Available: "+(available.empty()?"checking formats...":available),1200,258,21,muted,670,2);
    const std::string rows[]={"Video quality: up to "+qualityLabel(p.height),"Playback rate: up to "+std::to_string(p.maxFps)+" fps",
        "Decoder: "+std::string(!p.softwareCodecs?"Native AVC":p.preferAv1?"AV1 first":"VP9 first"),
        "Audio language: "+std::string(p.audioLanguage=="en"?"English":"Original"),
        "UHD recovery: "+std::string(p.autoRecovery?"On":"Off"),"Playback stats: "+std::string(debug?"On":"Off"),"Decoder threads (software): "+std::to_string(p.decoderThreads),"Scaler: "+std::string(p.scalerThreads==2?"2 threads (parallel)":"1 thread (baseline)"),"Apply changes","Close"};
    for(int i=0;i<10;++i) {const int y=318+i*55;c.rect(1186,y,708,48,i==selected?white:0xff363636);c.text(rows[i],1206,y+8,23,i==selected?0xff111111:white,665);}
    c.text("1080p supported target; higher resolutions unsupported.",1200,932,18,muted,670);
    c.text("LEFT / RIGHT  Change     CROSS  Select",1200,972,21,muted,670);
    c.text("CIRCLE  Close     Changes apply to this video",1200,1010,21,muted,670);
}
void drawQuality(Canvas& c,int selected,int actualHeight) {
    c.rect(1260,175,600,695,0xff222222);c.text("Quality for this video",1300,200,32,white,520);
    c.text("Playing "+std::to_string(actualHeight)+"p",1300,247,22,muted,520);
    for(int i=0;i<QualityCount;++i) {
        c.rect(1290,287+i*65,540,57,i==selected?white:0xff363636);
        c.text("Up to "+qualityLabel(qualityHeight(i)),1310,299+i*65,25,i==selected?0xff111111:white,500);
    }
    c.text("CROSS  Apply     CIRCLE  Cancel",1300,827,21,muted,530);
}
void drawLoading(Canvas& c,const Video& video,const std::string& status) {
    c.rect(0,0,1920,1080,background);c.rect(68,70,50,34,red);c.playIcon(85,78,17,white);
    c.text("Preparing playback",150,63,35,white,1650);c.text(video.title,130,360,42,white,1650,2);
    c.text(status,130,530,28,muted,1650,3);c.text("CIRCLE  Cancel     OPTIONS  Exit",130,938,25,muted,1650);
}
}

