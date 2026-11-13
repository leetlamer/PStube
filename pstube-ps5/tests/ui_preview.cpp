#include "shared/app_ui.h"
#include "network_ui.h"
#include "seek_ui.h"
#include <cstdio>
#include <cassert>
static void save(const char* name,const std::vector<uint32_t>& pixels) {FILE* file=fopen(name,"wb");assert(file);fprintf(file,"P6\n1920 1080\n255\n");for(auto p:pixels) {unsigned char rgb[]={(unsigned char)(p>>16),(unsigned char)(p>>8),(unsigned char)p};fwrite(rgb,1,3,file);}fclose(file);}
int main() {
    smarttube::CarouselMotion motion;assert(motion.step(0,1)==0);assert(motion.step(2,2)==0);assert(std::abs(motion.step(2,2.1)-1)<0.00001);double before=motion.value(2.1);assert(motion.step(3,2.1)==before);assert(std::abs(motion.step(3,2.31)-3)<0.00001);assert(smarttube::carouselTarget(9,10)==6);
    smarttube::Canvas canvas;assert(canvas.loadFont("assets/Gontserrat-Regular.ttf"));assert(canvas.loadIcons()==25);std::vector<uint32_t> pixels(1920*1080);canvas.surface({pixels.data(),1920,1080,1920});smarttube::Library library;smarttube::BrowseView view;view.library=&library;view.menu=smarttube::Channels;view.heading="Channels";view.sidebar=false;view.channelList=true;view.subtitle="Native PS5 beta";
    for(int i=0;i<12;++i) view.videos.push_back({"fixture"+std::to_string(i),"Channel "+std::to_string(i+1),"Open channel videos",""});smarttube::drawBrowse(canvas,view);save("build/beta-channels.ppm",pixels);
    view.channelList=false;view.menu=smarttube::Search;view.heading="Search results";for(auto& v:view.videos) {v.title="Nature and wildlife in 4K";v.channel="PStube test fixture";}smarttube::drawBrowse(canvas,view);save("build/beta-results.ppm",pixels);
    view.description="A longer video description from the creator, scrolling right to left while this video stays selected. Full descriptions arrive in the background without blocking thumbnail selection.";view.selectedSeconds=6;view.videos[0].title="An extended video title that should move smoothly from right to left when selected";
    view.shelves={{"Recommended",0,4,0},{"Music",4,4,0},{"Live now",8,4,0}};view.heading="Home";view.menu=smarttube::Home;smarttube::drawBrowse(canvas,view);save("build/beta-shelves.ppm",pixels);
    view.shelves.clear();view.menu=smarttube::Search;view.heading="Search results";view.sidebarExpansion=0;view.verticalOffset=0;
    for(auto& item:view.videos) {item.channel="Creator · 12K subscribers";item.views="123K views";item.published="2 days ago";item.likes="4.5K";}
    smarttube::drawBrowse(canvas,view);assert(pixels[300*1920+1800]!=0xff000000);save("build/beta-search-four.ppm",pixels);
    view.shelves={{"Recommended",0,4,0},{"Music",4,4,0},{"Live now",8,4,0}};view.menu=smarttube::Home;view.heading="Home";
    view.sidebarExpansion=0;view.verticalOffset=0;smarttube::drawBrowse(canvas,view);save("build/beta-hidden-sidebar.ppm",pixels);
    view.shelves.clear();view.menu=smarttube::Shorts;view.heading="Shorts";view.shorts=true;smarttube::drawBrowse(canvas,view);save("build/beta-shorts.ppm",pixels);
    view.shorts=false;view.sidebarExpansion=0.5;view.verticalOffset=0.5;smarttube::drawBrowse(canvas,view);save("build/beta-slide.ppm",pixels);view.verticalOffset=-1;view.sidebarExpansion=-1;
    probe::NetworkPreferences network;network.value.dns_mode=1;probe::drawNetworkSettings(canvas,network,0,"Test a route before saving it. Proxy is Off.");save("build/beta-network.ppm",pixels);
    smarttube::drawBrowse(canvas,view);
    canvas.rect(0,750,1920,330,0xff0c2036);probe::drawSeekTimeline(canvas,1920,750,1,180,600,true,{{60,90,0},{120,150,1},{0,20,2},{560,600,3},{250,270,4}});assert(pixels[975*1920+250]==0xff23c552 && pixels[975*1920+450]==0xfff2c94c && pixels[975*1920+70]==0xff56ccf2 && pixels[975*1920+1800]==0xff4361ee && pixels[975*1920+830]==0xffff69b4);save("build/beta-seek.ppm",pixels);
    smarttube::drawAccountOptions(canvas,0,"","Guest",-1,0);save("build/beta-accounts.ppm",pixels);
    std::fill(pixels.begin(),pixels.end(),0xff102030);canvas.marquee("A long UTF-8 title with café and many more words moving across a clipped window",100,100,24,0xffffffff,120,0);auto still=pixels;canvas.rect(100,100,120,45,0xff102030);canvas.marquee("A long UTF-8 title with café and many more words moving across a clipped window",100,100,24,0xffffffff,120,5);assert(pixels!=still);for(int y=0;y<1080;++y) for(int x=0;x<1920;++x) if(x<100 || x>=220 || y<100 || y>=145) assert(pixels[(size_t)y*1920+x]==0xff102030);
    canvas.clip({100,100,120,45});canvas.roundedRect({60,60,250,180},20,0xffffffff);for(int y=0;y<1080;++y) for(int x=0;x<1920;++x) if(x<100 || x>=220 || y<100 || y>=145) assert(pixels[(size_t)y*1920+x]==0xff102030);canvas.clip({0,0,1920,1080});
    std::fill(pixels.begin(),pixels.end(),0xff102030);canvas.clip({320,165,1580,760});canvas.origin(-288);canvas.icon("lb_ic_thumb_up",{300,300,64,64});canvas.icon("lb_ic_thumb_down",{590,300,64,64});for(int y=0;y<1080;++y) for(int x=0;x<320;++x) assert(pixels[(size_t)y*1920+x]==0xff102030);canvas.origin(0);canvas.clip({0,0,1920,1080});
    smarttube::drawLoadingSpinner(canvas,1920,1080,0);auto spinner=pixels;smarttube::drawLoadingSpinner(canvas,1920,1080,0.5);assert(pixels!=spinner && pixels[0]==0xff000000);
    smarttube::drawAccount(canvas,"Approve this code on your device","TEST-CODE","https://www.google.com/device");save("build/beta-pairing.ppm",pixels);
}
