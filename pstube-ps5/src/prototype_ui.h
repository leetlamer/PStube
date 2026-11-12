#pragma once
// Included inside platform's private namespace: the portable UI owns no SDK calls.
void runPrototype(Video& video,Controller& controller,Audio& audio,bool smoke,const char* smokeMode) {
    auto* canvas=new smarttube::Canvas;canvas->surface({video.linear,1920,1080,1920});
    if(!canvas->loadFont("/data/homebrew/PStube-PS5-Test/assets/Gontserrat-Regular.ttf")) {logLine("UI font_failed");delete canvas;return;}
    logLine("UI imported_icons=%u",canvas->loadIcons());
    probe::NetworkPreferences networkPreferences;networkPreferences.load("/data/pstube-ps5-network.json");networkPreferences.apply();
    smarttube::Library library;library.load("/data/pstube-ps5-library.json");
    bool captureTest=!strcmp(smokeMode,"--browse-capture");bool browseTest=!strcmp(smokeMode,"--browse-test") || !strcmp(smokeMode,"--browse-dearrow"),menuTest=!strcmp(smokeMode,"--beta-test") || !strcmp(smokeMode,"--dearrow-test") || browseTest || captureTest,networkTest=!strcmp(smokeMode,"--network-test");
    auto save=[&]() {if(!smoke && !menuTest && !networkTest && !library.save("/data/pstube-ps5-library.json")) logLine("APP library_save_failed");};
    if(!strcmp(smokeMode,"--smoke-services") || !strcmp(smokeMode,"--smoke-sponsor-real")) {library.preferences.sponsorMute=true;library.preferences.dearrow=true;}
    if(!strcmp(smokeMode,"--browse-dearrow") || !strcmp(smokeMode,"--dearrow-test")) {library.preferences.dearrow=true;library.preferences.dearrowTitles=library.preferences.dearrowThumbnails=true;}
    if(smoke) {library.preferences.height=strstr(smokeMode,"1440")?1440:(strstr(smokeMode,"2160") || !strcmp(smokeMode,"--smoke-native4k"))?2160:1080;library.preferences.native4k=!strcmp(smokeMode,"--smoke-native4k");}
    if(strstr(smokeMode,"threads10")) library.preferences.decoderThreads=10;
    uint64_t flips=0;std::string status="Loading PStube...";std::string query,visitor,continuation;int page=0;
    std::vector<smarttube::Shelf> shelves;bool shelfLayout=false,reelFeed=false;
    std::vector<smarttube::Video> items;std::map<std::string,std::shared_ptr<smarttube::Image>> images;size_t selected=0;std::map<std::string,bool> thumbnailAttempted;size_t thumbnailPage=SIZE_MAX;
    int menu=smarttube::Home;bool sidebar=true,accountOptions=false,pairingView=false,busy=false;int accountRow=0;
    std::string browseId,browseParams;bool channelList=false,playlistList=false,liveFeed=false;probe::BetaAccount* account=nullptr;
    smarttube::Keyboard keyboard;bool editing=false,quit=false,queuedPlay=false;uint32_t previous=controller.read();
    probe::DocumentJob descriptionJob;std::map<std::string,std::string> descriptions;std::string descriptionId,marqueeId;unsigned descriptionStage=0;double marqueeSince=probe::benchmarkTime();
    std::map<std::string,smarttube::CarouselMotion> carousel;unsigned previousUiState=UINT32_MAX;
    auto screen=[&]() {
        unsigned uiState=(unsigned)(menu+1)+(sidebar?32:0)+(editing?64:0)+(accountOptions?128:0)+(pairingView?256:0)+(unsigned)accountRow*512;
        if(uiState!=previousUiState) {logLine("UI view menu=%d sidebar=%d keyboard=%d accounts=%d pairing=%d account_row=%d",menu,sidebar,editing,accountOptions,pairingView,accountRow);previousUiState=uiState;}
        if(!video.linear) {quit=true;return;}
        canvas->rect(0,0,1920,1080,0xff081321);
        if(editing) smarttube::drawKeyboard(*canvas,keyboard);
        else {
            smarttube::BrowseView view;view.menu=menu;view.sidebar=sidebar;view.heading=query.empty()?smarttube::menuLabel(menu):query;
            view.subtitle="Browse videos and channels";view.status=status;view.videos=items;view.thumbnails=images;
            std::string focus=!sidebar && selected<items.size()?items[selected].id:std::string();if(focus!=marqueeId) {marqueeId=focus;marqueeSince=probe::benchmarkTime();}
            view.selectedSeconds=probe::benchmarkTime()-marqueeSince;if(descriptions.count(focus)) view.description=descriptions[focus];
            view.shelves=shelves;for(const auto& shelf:shelves) {size_t cursor=selected>=shelf.first && selected<shelf.first+shelf.count?selected-shelf.first:shelf.cursor;std::string key=shelf.title+(shelf.first<items.size()?items[shelf.first].id:"");view.shelfOffsets.push_back(carousel[key].step(smarttube::carouselTarget(cursor,shelf.count),probe::benchmarkTime()));}view.library=&library;view.selected=selected;view.busy=busy;view.channelList=channelList;
            if(account && !captureTest) {view.linked=account->oauth.linked();view.accountName=account->profile.name;view.accountImage=account->avatar;}
            smarttube::drawBrowse(*canvas,view);
            if(accountOptions && account) smarttube::drawAccountOptions(*canvas,accountRow,account->profile.name,account->oauth.status,account->slots.preferred,(unsigned)account->slots.slots.size());
            if(pairingView && account) smarttube::drawAccount(*canvas,account->oauth.status,account->oauth.userCode,account->oauth.verificationUrl);
        }
        for(size_t i=0;i<1920u*1080u;++i) {uint32_t p=video.linear[i];video.linear[i]=(p&0xff00ff00u)|((p&255u)<<16)|((p>>16)&255u);}
        if(!video.frame(flips%2,flips)) quit=true;else ++flips;
    };
    std::function<void()> networkPanel;
    auto sponsorPanel=[&]() {
        int row=0;uint32_t previous=controller.read();bool showing=true;
        while(showing && !quit) {
            unsigned scale=video.width/1920;int left=video.width*2/5;canvas->surface({video.linear,(int)video.width,(int)video.height,(int)video.width});canvas->rect(left,0,video.width-left,video.height,0xff102941);
            canvas->text("SponsorBlock",left+40*scale,45*scale,34*scale,0xffeef6ff,1000*scale);
            const char* categories[]={"Sponsors","Self promotion","Introductions","Outros","Interaction reminders"};
            std::vector<std::string> rows={std::string("Automatic skipping: ")+(library.preferences.sponsorMute?"On":"Off")};
            for(unsigned i=0;i<5;++i) rows.push_back(std::string(categories[i])+": "+((library.preferences.sponsorCategories&(1u<<i))?"Skip":"Keep"));
            rows.push_back("Minimum segment: "+std::to_string(library.preferences.sponsorMinimum)+" seconds");rows.push_back(std::string("Skip notices: ")+(library.preferences.sponsorNotices?"On":"Off"));rows.push_back("Back");
            for(int i=0;i<9;++i) {int y=(140+i*76)*scale;canvas->rect(left+24*scale,y,video.width-left-48*scale,62*scale,i==row?0xff329dff:0xff173550);canvas->text(rows[i],left+42*scale,y+16*scale,25*scale,0xffeef6ff,video.width-left-80*scale);}
            canvas->text("Changes apply to the next video. Community timings may vary.",left+35*scale,900*scale,22*scale,0xff9db4cc,video.width-left-70*scale,2);
            for(unsigned y=0;y<video.height;++y) for(unsigned x=left;x<video.width;++x) {auto& p=video.linear[(size_t)y*video.width+x];p=(p&0xff00ff00u)|((p&255u)<<16)|((p>>16)&255u);}
            if(!video.frame(flips%2,flips)) {quit=true;break;}++flips;uint32_t buttons=controller.read(),pressed=buttons&~previous;previous=buttons;
            if(pressed&PS5_PAD_BUTTON_OPTIONS) {quit=true;break;}if(pressed&PS5_PAD_BUTTON_CIRCLE) break;
            if(pressed&PS5_PAD_BUTTON_UP && row) --row;if(pressed&PS5_PAD_BUTTON_DOWN && row<10) ++row;
            if(pressed&(PS5_PAD_BUTTON_LEFT|PS5_PAD_BUTTON_RIGHT|PS5_PAD_BUTTON_CROSS)) {
                if(row==0) library.preferences.sponsorMute=!library.preferences.sponsorMute;
                if(row>=1 && row<=5) library.preferences.sponsorCategories^=1u<<(row-1);
                if(row==6) {unsigned values[]={0,1,3,5};unsigned index=0;while(index<3 && values[index]!=library.preferences.sponsorMinimum) ++index;library.preferences.sponsorMinimum=values[(index+((pressed&PS5_PAD_BUTTON_LEFT)?3:1))%4];}
                if(row==7) library.preferences.sponsorNotices=!library.preferences.sponsorNotices;if(row==8) showing=false;save();
            }
        }
    };
    auto settingsPanel=[&](bool playing) {
        int row=0;uint32_t previous=controller.read();bool running=true;
        while(running && !quit) {
            unsigned w=video.width,h=video.height,scale=w/1920;int left=(int)w*2/5;
            canvas->surface({video.linear,(int)w,(int)h,(int)w});canvas->rect(left,0,w-left,h,0xff102941);
            canvas->text("Playback settings",left+45*scale,55*scale,34*scale,0xffeef6ff,w-left-70*scale);
            std::string rows[]={"Source quality: "+smarttube::qualityLabel(library.preferences.height),std::string("Display output: ")+(library.preferences.native4k?"2160p (experimental)":"1080p"),"Frame rate: up to "+std::to_string(library.preferences.maxFps),std::string("After video: ")+smarttube::endModeLabel(library.preferences.endMode),std::string("Debug metrics: ")+(library.preferences.debugMetrics?"On":"Off"),(playing?"Network settings (Home only)":"Network settings"),std::string("SponsorBlock settings: ")+(library.preferences.sponsorMute?"On":"Off"),std::string("DeArrow: ")+(!library.preferences.dearrow?"Off":library.preferences.dearrowTitles && library.preferences.dearrowThumbnails?"Titles + thumbnails":library.preferences.dearrowTitles?"Titles only":"Thumbnails only"),"Decoder threads: "+std::to_string(library.preferences.decoderThreads),"HDR output: unavailable (SDR)","Back"};
            for(int i=0;i<11;++i) {int y=(145+i*60)*scale;canvas->rect(left+24*scale,y,w-left-48*scale,54*scale,i==row?0xff329dff:0xff173550);canvas->text(rows[i],left+42*scale,y+18*scale,27*scale,0xffeef6ff,w-left-75*scale);}
            canvas->text(playing?"Next video/browse applies changes. SponsorBlock skips marked sponsors.":"Optional services share video IDs with sponsor.ajay.app. Circle returns.",left+35*scale,850*scale,23*scale,0xff9db4cc,w-left-60*scale,2);
            for(unsigned y=0;y<h;++y) for(unsigned x=left;x<w;++x) {auto& p=video.linear[(size_t)y*w+x];p=(p&0xff00ff00u)|((p&255u)<<16)|((p>>16)&255u);}
            if(!video.frame(flips%2,flips)) return false;++flips;
            uint32_t b=controller.read(),pressed=b&~previous;previous=b;
            if(pressed&PS5_PAD_BUTTON_OPTIONS) {quit=true;return false;}
            if(pressed&PS5_PAD_BUTTON_CIRCLE) running=false;
            if((pressed&PS5_PAD_BUTTON_UP) && row) --row;if((pressed&PS5_PAD_BUTTON_DOWN) && row<8) ++row;
            if(pressed&(PS5_PAD_BUTTON_LEFT|PS5_PAD_BUTTON_RIGHT|PS5_PAD_BUTTON_CROSS)) {
                int direction=(pressed&PS5_PAD_BUTTON_LEFT)?-1:1;
                if(row==0) library.preferences.height=smarttube::cycleQuality(library.preferences.height,direction);
                if(row==1) library.preferences.native4k=!library.preferences.native4k;
                if(row==2) library.preferences.maxFps=library.preferences.maxFps==60?30:60;
                if(row==3) library.preferences.endMode=smarttube::nextEndMode(library.preferences.endMode);
                if(row==4) library.preferences.debugMetrics=!library.preferences.debugMetrics;
                if(row==5 && !playing && networkPanel) networkPanel();if(row==6) sponsorPanel();if(row==7) {int mode=!library.preferences.dearrow?0:library.preferences.dearrowTitles && library.preferences.dearrowThumbnails?1:library.preferences.dearrowTitles?2:3;mode=(mode+(direction<0?3:1))%4;library.preferences.dearrow=mode!=0;library.preferences.dearrowTitles=mode==1 || mode==2;library.preferences.dearrowThumbnails=mode==1 || mode==3;}if(row==8) {unsigned n=library.preferences.decoderThreads;library.preferences.decoderThreads=direction<0?(n==6?10:n==8?6:8):(n==10?6:n==8?10:8);}
                if(row==9) logLine("HDR unavailable: verified 8-bit SDR framebuffer; HDR10 output/metadata path not implemented");if(row==10) running=false;save();
            }
        }
        logLine("APP settings height=%d native4k=%d max_fps=%d end_mode=%d debug=%d",library.preferences.height,library.preferences.native4k,library.preferences.maxFps,(int)library.preferences.endMode,library.preferences.debugMetrics);return !quit;
    };
    screen();
    void* net=loadModule("libSceNet.sprx");auto init=reinterpret_cast<int(*)()>(net?dlsym(net,"sceNetInit"):nullptr);if(init) logLine("APP net_init rc=0x%08x",(unsigned)init());
    std::function<void()> fetchPaint;const console_network_settings* requestNetworkOverride=nullptr;
    bool global=curl_global_init(CURL_GLOBAL_DEFAULT)==0;logLine("APP started smoke=%d curl_global=%d",smoke,global);
    // Network work runs off-thread while menus continue drawing and cancellation remains responsive.
    auto fetch=[&](const smarttube::Request& request,size_t limit)->probe::Document {
        struct Task {smarttube::Request request;size_t limit;probe::Document result;std::atomic<bool> done{false},cancelled{false};console_network_settings network{};bool overrideNetwork=false;};
        if(quit) return {"","Cancelled"};
        Task task;task.request=request;task.limit=limit;if(requestNetworkOverride) {task.network=*requestNetworkOverride;task.overrideNetwork=true;}pthread_t thread{};pthread_attr_t attr;
        if(pthread_attr_init(&attr)) return {"","Cannot start network worker"};
        int rc=pthread_attr_setstacksize(&attr,2*1024*1024);
        if(!rc) rc=pthread_create(&thread,&attr,+[](void* p)->void* {auto& t=*static_cast<Task*>(p);t.result=probe::performRequest(t.request,t.limit,30,&t.cancelled,nullptr,t.overrideNetwork?&t.network:nullptr);t.done=true;return nullptr;},&task);
        pthread_attr_destroy(&attr);if(rc) return {"","Cannot start network worker"};
        bool wasBusy=busy;busy=true;uint32_t last=controller.read();
        while(!task.done) {if(fetchPaint) fetchPaint();else screen();uint32_t buttons=controller.read(),pressed=buttons&~last;last=buttons;
            if(pressed&(PS5_PAD_BUTTON_CIRCLE|PS5_PAD_BUTTON_OPTIONS)) {task.cancelled=true;if(pressed&PS5_PAD_BUTTON_OPTIONS) quit=true;}
            if(quit) task.cancelled=true;
        }
        pthread_join(thread,nullptr);busy=wasBusy;previous=controller.read();
        if(task.cancelled) task.result.error="Cancelled";return task.result;
    };
    networkPanel=[&]() {
        probe::NetworkPreferences draft=networkPreferences;int row=0;std::string note="Changes apply after Save. System PS5 settings stay unchanged.";
        auto flip=[&]() {for(size_t i=0;i<1920u*1080u;++i) {auto& p=video.linear[i];p=(p&0xff00ff00u)|((p&255u)<<16)|((p>>16)&255u);}if(!video.frame(flips%2,flips)) quit=true;else ++flips;};
        auto edit=[&](const std::string& label,std::string value,bool secret,size_t maximum)->std::pair<bool,std::string> {
            int r=0,c=0;bool upper=false;uint32_t last=controller.read();double repeat=0;
            const std::string keys[]={"1234567890.-","qwertyuiop[]","asdfghjkl:;_","zxcvbnm,/?@+","!#$%&*()=~\\"};
            while(!quit) {
                canvas->surface({video.linear,1920,1080,1920});canvas->rect(0,0,1920,1080,0xff081321);canvas->text(label,270,70,38,0xffeef6ff,1400);
                std::string shown=secret?std::string(value.size(),'*'):value;if(shown.size()>72) shown="..."+shown.substr(shown.size()-72);canvas->rect(270,160,1390,85,0xff173550);canvas->text(shown+"|",290,183,27,0xffeef6ff,1350);
                for(int y=0;y<6;++y) {int count=y==5?4:(int)keys[y].size();for(int x=0;x<count;++x) {int width=y==5?320:100,xx=270+x*(y==5?350:112),yy=315+y*95;canvas->rect(xx,yy,width,76,r==y && c==x?0xff329dff:0xff19334e);std::string text=y==5?(x==0?"Space":x==1?"Delete":x==2?"Clear":"Done"):keys[y].substr(x,1);if(upper && text.size()==1 && text[0]>='a' && text[0]<='z') text[0]-=32;canvas->text(text,xx+20,yy+20,28,0xffeef6ff,width-30);}}
                canvas->text("R1  Done   L1  Delete   SQUARE  Upper/lower case   CIRCLE  Cancel",270,965,24,0xff9db4cc,1390);flip();
                uint32_t buttons=controller.read(),pressed=buttons&~last;last=buttons;double now=probe::benchmarkTime();uint32_t directions=PS5_PAD_BUTTON_UP|PS5_PAD_BUTTON_DOWN|PS5_PAD_BUTTON_LEFT|PS5_PAD_BUTTON_RIGHT;
                if(pressed&directions) repeat=now+0.4;else if((buttons&directions) && now>=repeat) {pressed|=buttons&directions;repeat=now+0.13;}
                if(pressed&PS5_PAD_BUTTON_OPTIONS) {quit=true;return {false,value};}if(pressed&PS5_PAD_BUTTON_CIRCLE) return {false,value};if(pressed&PS5_PAD_BUTTON_R1) return {true,value};
                if(pressed&PS5_PAD_BUTTON_UP && r) --r;if(pressed&PS5_PAD_BUTTON_DOWN && r<5) ++r;int count=r==5?4:(int)keys[r].size();c=std::min(c,count-1);
                if(pressed&PS5_PAD_BUTTON_LEFT && c) --c;if(pressed&PS5_PAD_BUTTON_RIGHT && c+1<count) ++c;if(pressed&PS5_PAD_BUTTON_SQUARE) upper=!upper;if(pressed&PS5_PAD_BUTTON_L1 && !value.empty()) value.pop_back();
                if(pressed&PS5_PAD_BUTTON_CROSS) {if(r==5 && c==3) return {true,value};if(r==5 && c==1) {if(!value.empty()) value.pop_back();}else if(r==5 && c==2) value.clear();else if(value.size()<maximum) {char ch=r==5?' ':keys[r][c];if(upper && ch>='a' && ch<='z') ch-=32;value+=ch;}}
            }return {false,value};
        };
        auto draw=[&]() {canvas->surface({video.linear,1920,1080,1920});probe::drawNetworkSettings(*canvas,draft,row,note);flip();};
        uint32_t last=controller.read();bool showing=true;
        while(showing && !quit) {
            draw();uint32_t buttons=controller.read(),pressed=buttons&~last;last=buttons;
            if(pressed&PS5_PAD_BUTTON_OPTIONS) quit=true;if(pressed&PS5_PAD_BUTTON_CIRCLE) showing=false;
            if(pressed&PS5_PAD_BUTTON_UP && row) --row;if(pressed&PS5_PAD_BUTTON_DOWN && row<10) ++row;
            if(pressed&(PS5_PAD_BUTTON_LEFT|PS5_PAD_BUTTON_RIGHT|PS5_PAD_BUTTON_CROSS)) {
                int direction=pressed&PS5_PAD_BUTTON_LEFT?-1:1;
                if(row==0) draft.value.dns_mode=(draft.value.dns_mode+4+direction)%4;
                if(row==2) draft.value.proxy_mode=(draft.value.proxy_mode+4+direction)%4;
                if((pressed&PS5_PAD_BUTTON_CROSS) && (row==1 || (row>=3 && row<=6))) {
                    char* target=row==1?draft.value.doh_url:row==3?draft.value.proxy_host:row==5?draft.value.proxy_user:draft.value.proxy_password;
                    std::string value=row==4?std::to_string(draft.value.proxy_port):target;
                    auto result=edit(row==1?"Custom DNS over HTTPS URL":row==3?"Proxy hostname or IPv4 address":row==4?"Proxy port":row==5?"Proxy username":"Proxy password",value,row==6,row==1?512:row==3?253:row==4?5:128);
                    if(result.first) {if(row==4) {char* end=nullptr;long port=strtol(result.second.c_str(),&end,10);if(end && !*end && port>=1 && port<=65535) draft.value.proxy_port=(int)port;else note="Port must be between1 and65535";}else memcpy(target,result.second.c_str(),result.second.size()+1);}last=controller.read();
                }
                if(row==7 && (pressed&PS5_PAD_BUTTON_CROSS)) {
                    std::string error;if(!draft.valid(error)) note=error;else {
                        note="Testing verified HTTPS through the selected route...";smarttube::Request request;request.httpGet=true;request.url="https://www.youtube.com/generate_204";request.userAgent="PStube PS5";
                        auto oldPaint=fetchPaint;fetchPaint=draw;requestNetworkOverride=&draft.value;auto response=fetch(request,1024);requestNetworkOverride=nullptr;fetchPaint=oldPaint;
                        bool ok=response.status==204 && (response.error.empty() || response.error=="HTTP 204");note=ok?"Connection succeeded. Settings have not been saved.":"Connection failed. Check the DNS/proxy configuration.";
                        logLine("NETWORK test dns_mode=%d proxy_mode=%d HTTP=%ld ok=%d",draft.value.dns_mode,draft.value.proxy_mode,response.status,ok);last=controller.read();
                    }
                }
                if(row==8 && (pressed&PS5_PAD_BUTTON_CROSS)) {
                    std::string error;if(!draft.valid(error)) note=error;else if(!draft.save("/data/pstube-ps5-network.json")) note="Could not save network settings";else {networkPreferences=draft;networkPreferences.apply();note="Network settings applied to new requests";showing=false;logLine("NETWORK saved dns_mode=%d proxy_mode=%d",draft.value.dns_mode,draft.value.proxy_mode);}
                }
                if(row==9 && (pressed&PS5_PAD_BUTTON_CROSS)) showing=false;
                if(row==10 && (pressed&PS5_PAD_BUTTON_CROSS)) {draft=probe::NetworkPreferences{};note="Defaults staged. Save to use the system route.";}
                if(row==2) note=draft.value.proxy_mode?"The proxy resolves destination DNS. The DNS selector applies when Proxy is Off.":"Proxy Off. The selected app DNS will be used after Save.";
            }
        }
    };
    probe::BetaAccount accountState(fetch);account=&accountState;
    if(account->oauth.linked() && !captureTest) {status="Restoring account...";account->loadProfile();}
    probe::ThumbnailLoader thumbnailLoader;logLine("UI workers=%u image_workers=2 dearrow_worker=1 persistent_connections=1 decode=640x360",thumbnailLoader.workers());
    auto visibleItems=[&]() {
        std::vector<smarttube::Video> visible;
        if(shelves.empty()) {size_t first=smarttube::resultFirst(selected,channelList),count=channelList?6:16;for(size_t i=first;i<items.size() && i<first+count;++i) visible.push_back(items[i]);}
        else {size_t active=smarttube::selectedShelf(shelves,selected),top=active?active-1:0;
            auto add=[&](size_t row) {auto& shelf=shelves[row];size_t cursor=row==active?selected-shelf.first:shelf.cursor;size_t start=cursor>2?cursor-2:0;for(size_t col=start;col<shelf.count && col<start+6;++col) visible.push_back(items[shelf.first+col]);};
            add(active);for(size_t r=top;r<shelves.size() && r<top+2;++r) if(r!=active) add(r);
        }return visible;
    };
    auto thumbnails=[&]() {
        thumbnailLoader.dearrow(library.preferences.dearrow,library.preferences.dearrowTitles,library.preferences.dearrowThumbnails);auto visible=visibleItems();thumbnailLoader.visible(visible,0,visible.size(),images);size_t first=smarttube::resultFirst(selected,channelList),count=channelList?smarttube::ChannelRows:16;
        (void)count;thumbnailPage=first;
    };
    auto reset=[&]() {thumbnailLoader.clear();carousel.clear();shelves.clear();items.clear();reelFeed=false;continuation.clear();selected=0;images.clear();thumbnailAttempted.clear();thumbnailPage=SIZE_MAX;channelList=playlistList=false;};
    auto receive=[&](const smarttube::Request& request,bool append) {
        auto response=fetch(request,4*1024*1024);smarttube::Json document;std::string error;
        if(response.error.empty() && smarttube::parseJson(response.body,document,error)) {
            if(shelfLayout) {if(!append) smarttube::shelfCatalog(document,items,shelves);else smarttube::appendShelfPage(document,items,shelves);}
            auto found=shelfLayout?std::vector<smarttube::Video>{}:channelList?smarttube::channelCatalog(document):playlistList?smarttube::playlistCatalog(document):smarttube::catalog(document);
            for(const auto& item:found) {bool duplicate=false;for(const auto& old:items) if(old.id==item.id) duplicate=true;if(!duplicate && items.size()<1000) items.push_back(item);}
            auto next=shelfLayout?smarttube::feedContinuation(document):smarttube::catalogContinuation(document);if(next==continuation) next.clear();continuation=next;
            if(auto* context=document.get("responseContext")) visitor=context->string("visitorData");status=items.empty()?"No results. Sign in or try another search.":"Select an item to open.";
        } else status=response.error.empty()?error:response.error;
        size_t namedShelves=0;for(const auto& shelf:shelves) if(shelf.title!="Videos" && !shelf.title.empty()) ++namedShelves;logLine("UI category_headers named=%zu total=%zu",namedShelves,shelves.size());
        logLine("APP catalog menu=%d append=%d HTTP=%ld results=%zu continuation=%d shelves=%zu",menu,append,response.status,items.size(),!continuation.empty(),shelves.size());
        screen();
    };
    auto search=[&](const std::string& value) {
        page=0;menu=smarttube::Search;sidebar=false;reset();browseId.clear();browseParams.clear();liveFeed=false;shelfLayout=false;query=value;status="Searching YouTube...";screen();
        auto request=smarttube::searchRequest(value,"TVHTML5","7.20260901.15.00");request.clientNumber="7";request.userAgent=smarttube::tvUserAgent(0);
        receive(request,false);thumbnails();screen();
    };
    auto more=[&](bool vertical=false) {
        if(shelfLayout && !shelves.empty() && (vertical || shelves[smarttube::selectedShelf(shelves,selected)].continuation.empty())) {
            if(continuation.empty() || items.size()>=1000 || shelves.size()>=40) return;
            size_t before=items.size(),rows=shelves.size();receive(account->browse(browseId,continuation,browseParams,account->oauth.linked()),true);
            logLine("APP feed_more rows_added=%zu videos_added=%zu continuation=%d",shelves.size()-rows,items.size()-before,!continuation.empty());return;
        }
        if(shelfLayout && !shelves.empty()) {
            size_t row=smarttube::selectedShelf(shelves,selected);auto token=shelves[row].continuation;if(token.empty() || items.size()>=1000) return;
            auto response=fetch(account->browse(browseId,token,browseParams,account->oauth.linked()),4*1024*1024);smarttube::Json json;std::string error;
            if(response.error.empty() && smarttube::parseJson(response.body,json,error)) {
                size_t added=smarttube::appendShelfVideos(json,row,items,shelves);auto next=smarttube::catalogContinuation(json);shelves[row].continuation=next==token?std::string():next;
                logLine("APP shelf_more row=%zu added=%zu HTTP=%ld",row,added,response.status);
            }return;
        }
        if(reelFeed && !continuation.empty()) {
            auto request=smarttube::shortsRequest(continuation);request.userAgent=smarttube::tvUserAgent(0);auto response=fetch(request,4*1024*1024);smarttube::Json json;std::string error;
            if(smarttube::parseJson(response.body,json,error)) {auto found=smarttube::reelVideos(json);for(auto& item:found) if(items.size()<1000) {bool duplicate=false;for(const auto& old:items) if(old.id==item.id) duplicate=true;if(!duplicate) items.push_back(std::move(item));}auto next=smarttube::continuationIn(json);continuation=next==continuation?std::string():next;}return;
        }
        if(continuation.empty() || items.size()>=1000) return;status="Loading more results...";smarttube::Request request;
        if(!browseId.empty()) request=account->browse(browseId,continuation,browseParams,account->oauth.linked());
        else {request=smarttube::searchRequest(query,"TVHTML5","7.20260901.15.00");request.clientNumber="7";
            request.body=R"({"context":{"client":{"clientName":"TVHTML5","clientVersion":"7.20260901.15.00","hl":"en","gl":"US"}},"continuation":)"+smarttube::quoteJson(continuation)+"}";}
        receive(request,true);
    };
    auto openMenu=[&](int destination) {
        menu=destination;page=0;reset();shelfLayout=false;browseId.clear();browseParams.clear();query=smarttube::menuLabel(menu);liveFeed=false;sidebar=false;
        logLine("APP menu_open=%d",menu);
        if(menu==smarttube::Search) {editing=true;keyboard=smarttube::Keyboard{};return;}
        if(menu==smarttube::Favorites || menu==smarttube::LocalHistory) {items=menu==smarttube::Favorites?library.favorites:library.history;status="Saved on this console";return;}
        if(menu==smarttube::Settings) {settingsPanel(false);canvas->surface({video.linear,1920,1080,1920});return;}
        if(menu>=smarttube::Subscriptions && menu<=smarttube::Liked && !account->refresh()) {status="Select login at the top left to link your account.";sidebar=true;return;}
        const char* ids[]={"FEwhat_to_watch","","FEsubscriptions","FEsubscriptions","FEhistory","FEplaylist_aggregation","VLLL","","","FEtopics_live"};
        shelfLayout=menu==smarttube::Home || menu==smarttube::Music || menu==smarttube::Gaming || menu==smarttube::Live || menu==smarttube::News;
        
        if(menu<10) browseId=ids[menu];if(menu==smarttube::Home) browseId="default";if(menu==smarttube::Music) browseId="FEtopics_music";if(menu==smarttube::Gaming) browseId="FEtopics_gaming";if(menu==smarttube::News) browseId="FEtopics_news";if(menu==smarttube::Shorts) browseId="FEsubscriptions";channelList=menu==smarttube::Channels;playlistList=menu==smarttube::Playlists;liveFeed=menu==smarttube::Live;
        if(browseId.empty()) return;status="Loading "+query+"...";screen();receive(account->browse(browseId,"","",menu!=smarttube::Live && account->oauth.linked()),false);
        if(menu==smarttube::Shorts) {
            auto request=account->browse("FEsubscriptions");auto reply=fetch(request,4*1024*1024);smarttube::Json json;std::string error;std::vector<smarttube::Video> shortsItems;std::vector<smarttube::Shelf> groups;items.clear();
            if(smarttube::parseJson(reply.body,json,error)) {smarttube::shelfCatalog(json,shortsItems,groups);for(const auto& group:groups) if(group.title.find("Short")!=std::string::npos) for(size_t i=group.first;i<group.first+group.count;++i) items.push_back(shortsItems[i]);}
            if(items.empty()) {
                auto request=smarttube::shortsRequest();request.userAgent=smarttube::tvUserAgent(0);auto reply=fetch(request,4*1024*1024);
                if(smarttube::parseJson(reply.body,json,error)) {items=smarttube::reelVideos(json);continuation=json.string("sequenceContinuation");reelFeed=true;}
            }
            logLine("APP shorts reel=%d results=%zu continuation=%d",reelFeed,items.size(),!continuation.empty());shelves.clear();browseId.clear();status=items.empty()?"Shorts feed unavailable for this client. Try again later.":"Shorts · Cross plays; R1 loads more";if(reelFeed && !continuation.empty()) more();
        }
        if(liveFeed && items.empty()) {auto request=smarttube::searchRequest("live","TVHTML5","7.20260901.15.00");request.clientNumber="7";auto at=request.body.rfind('}');if(at!=std::string::npos) request.body.insert(at,",\"params\":\"EgJAAQ%3D%3D\"");receive(request,false);}
    };
    auto play=[&](const smarttube::Video& item,bool local,unsigned limit) {
        descriptionJob.cancel();thumbnailLoader.clear();status="Preparing playback...";screen();smarttube::PlayerFeatures features;std::string url,audioUrl,agent,error;bool isLive=false;unsigned duration=0;int sourceHeight=0,sourceFps=0;
        auto resolvePlayback=[&]() {url.clear();audioUrl.clear();isLive=false;for(unsigned profile=0;profile<smarttube::AnonymousProfiles;++profile) {
            auto request=smarttube::anonymousPlayerRequest(item.id,profile,visitor);auto response=fetch(request,4*1024*1024);smarttube::Json json;
            if(!response.error.empty()) {error=response.error;continue;}if(!smarttube::parseJson(response.body,json,error)) continue;
            features.player(json);auto info=smarttube::playerInfo(json);logLine("APP player profile=%s status=%s formats=%zu reason=%s",smarttube::anonymousProfileName(profile),info.status.c_str(),info.streams.size(),info.reason.c_str());
            duration=info.durationMs;
            if((info.live || !duration) && !info.hls.empty()) {
                smarttube::Request master;master.httpGet=true;master.url=info.hls;master.userAgent=request.userAgent;
                auto response=fetch(master,4*1024*1024);smarttube::LiveHlsVariant variant;smarttube::LiveHlsDiagnostics diagnostics;
                if(response.error.empty() && smarttube::selectLiveHls(response.body,info.hls,library.preferences.height,library.preferences.maxFps,variant,error,&diagnostics)) {
                    sourceHeight=variant.height;sourceFps=variant.fps;url=variant.url;audioUrl=variant.audioUrl;agent=request.userAgent;isLive=true;
                    logLine("APP chosen live height=%u fps=%u separate_audio=%d variants=%u",variant.height,variant.fps,!audioUrl.empty(),diagnostics.variants);break;
                }
                logLine("APP live_manifest HTTP=%ld variants=%u accepted=%u",response.status,diagnostics.variants,diagnostics.accepted);
                continue;
            }
            smarttube::DecoderCaps caps;caps.avc=true;caps.aac=true;auto choice=smarttube::chooseStream(info,caps,library.preferences.height);
            auto adaptive=probe::adaptiveChoice(info,library.preferences.height,library.preferences.maxFps);int videoTrack=adaptive.video,audioTrack=adaptive.audio;

            if(videoTrack>=0 && audioTrack>=0) {sourceHeight=info.streams[videoTrack].height;sourceFps=info.streams[videoTrack].fps;url=info.streams[videoTrack].url;audioUrl=info.streams[audioTrack].url;agent=request.userAgent;logLine("APP chosen adaptive_video=%d adaptive_audio=%d height=%d fps=%d",info.streams[videoTrack].itag,info.streams[audioTrack].itag,info.streams[videoTrack].height,info.streams[videoTrack].fps);break;}
            if(choice.index>=0) {sourceHeight=info.streams[choice.index].height;sourceFps=info.streams[choice.index].fps;url=info.streams[choice.index].url;agent=request.userAgent;logLine("APP chosen itag=%d height=%d",info.streams[choice.index].itag,info.streams[choice.index].height);break;}
            error=info.reason.empty()?"No direct combined audio/video format available":info.reason;
        }
        };if(!local) resolvePlayback();
        if(!local && url.empty()) {status=error;logLine("APP playback_unavailable=%s",error.c_str());return;}
        if(!local) {auto watched=item;if(!features.title.empty()) watched.title=features.title;if(!features.channelName.empty()) watched.channel=features.channelName;library.watched(watched);save();}
        logLine("APP media_duration_ms=%u live=%d",duration,isLive);if(local) duration=12000;
        unsigned recoveryCount=0;double startSeconds=0,requestedSeek=-1;std::string pendingNotice;unsigned seekTestStage=0;bool realSponsorPositioned=false;std::vector<probe::MuteRange> sponsorSegments,sponsorMarkers;bool sponsorFetched=false;int retainedCaption=-1,retainedDim=0;std::vector<smarttube::CaptionCue> retainedCues;
        for(;;) {requestedSeek=-1;bool skipToEnd=false;
        unsigned outWidth=library.preferences.native4k?3840:1920,outHeight=library.preferences.native4k?2160:1080;
        if(!video.mode(outWidth,outHeight)) {logLine("APP native_output_failed fallback1080=1");if(!video.mode(1920,1080)) {quit=true;return;}}
        video.tileMs=video.submitMs=video.waitMs=video.lastFlip=video.intervalTotal=video.maxInterval=0;video.intervalCount=video.longIntervals=0;
        AudioSinkState sink{&audio};if(audio.ready) sink.handle=audio.sceAudioOutOpen(0xff,0,0,1024,48000,1);
        if(sink.handle<=0) {status="Audio device unavailable";if(!video.mode(1920,1080)) {quit=true;return;}canvas->surface({video.linear,1920,1080,1920});return;}
        auto* pcm=new probe::PcmQueue(outputPcm,&sink,logLine);pcm->pause(!audioUrl.empty());bool started=pcm->start();
        probe::VideoStreamMetrics playbackMetrics;PlaybackDisplay output{&video,&controller};output.positionMs=startSeconds*1000;output.pictureDim=retainedDim;output.pcm=pcm;output.previous=controller.read();
        output.settings=[&]() {bool result=settingsPanel(true);output.quit=quit;return result;};
        bool controlFocus=false,discussionOpen=false,chatMode=isLive;double seekPreview=-1;int controlRow=0,controlColumn=0,captionIndex=-1;size_t discussionOffset=0;
        captionIndex=retainedCaption;std::vector<smarttube::CaptionCue> cues=retainedCues;std::vector<smarttube::DiscussionLine> lines;std::shared_ptr<smarttube::Image> channelImage;
        std::string featureStatus,discussionToken,openChannel,captionAgent=agent;double toastUntil=pendingNotice.empty()?0:probe::benchmarkTime()+5,nextChatPoll=0;featureStatus=pendingNotice;pendingNotice.clear();bool metadataReady=false,discussionAttempted=false;int metadataStage=0;
        probe::DocumentJob metadataJob,discussionJob,sponsorJob;
        if(library.preferences.sponsorMute && library.preferences.sponsorCategories && !sponsorFetched && !local && !isLive) {smarttube::Request request;request.httpGet=true;request.url="https://sponsor.ajay.app/api/skipSegments?categories="+probe::sponsorQuery(library.preferences.sponsorCategories)+"&actionTypes=%5B%22skip%22%5D&videoID="+item.id;request.userAgent="PStube PS5";sponsorJob.start(request);}
        auto auth=[&]() {return account->oauth.expiresAt>(uint64_t)time(nullptr)?"Bearer "+account->oauth.accessToken:std::string();};
        auto nextRequest=smarttube::featureRequest("next","\"videoId\":"+smarttube::quoteJson(item.id),auth());nextRequest.userAgent=smarttube::tvUserAgent(0);
        if(!local) metadataJob.start(nextRequest);
        auto nativeRect=[&](unsigned x,unsigned y,unsigned w,unsigned h) {for(unsigned yy=y;yy<std::min(video.height,y+h);++yy) for(unsigned xx=x;xx<std::min(video.width,x+w);++xx) {auto& pixel=video.linear[(size_t)yy*video.width+xx];pixel=(pixel&0xff00ff00u)|((pixel&255u)<<16)|((pixel>>16)&255u);}};
        auto toast=[&](const std::string& message) {featureStatus=message;toastUntil=probe::benchmarkTime()+5;};
        fetchPaint=[&]() {unsigned scale=video.width/1920;canvas->surface({video.linear,(int)video.width,(int)video.height,(int)video.width});int x=1000*scale,y=220*scale;canvas->rect(x,y,870*scale,210*scale,0xff102941);canvas->text("Contacting YouTube...",x+30*scale,y+40*scale,30*scale,0xffeef6ff,800*scale);canvas->text("CIRCLE  Cancel",x+30*scale,y+110*scale,24*scale,0xff9db4cc,800*scale);nativeRect(x,y,870*scale,210*scale);if(video.frame(output.flips%2,output.flips)) ++output.flips;else quit=true;};
        auto requestAction=[&](const std::string& endpoint,const std::string& fields) {
            if(smoke || menuTest) {toast("Account changes disabled during automated diagnostics");return false;}
            if(!account->refresh()) {toast("Login to use this action");return false;}
            auto request=smarttube::featureRequest(endpoint,fields,"Bearer "+account->oauth.accessToken);request.userAgent=smarttube::tvUserAgent(0);
            auto reply=fetch(request,2*1024*1024);smarttube::Json document;std::string error;
            bool ok=reply.status==200 && reply.error.empty() && smarttube::parseJson(reply.body,document,error) && !document.get("error") && document.get("responseContext");
            logLine("PLAYER account_action endpoint=%s HTTP=%ld accepted=%d",endpoint.c_str(),reply.status,ok);if(!ok) toast(reply.error.empty()?"YouTube did not confirm this action":reply.error);return ok;
        };
        auto choosePlaylist=[&]() {
            if(!account->refresh()) {toast("Login to add a video to a playlist");return;}
            auto request=smarttube::featureRequest("playlist/get_add_to_playlist","\"videoIds\":["+smarttube::quoteJson(item.id)+"]","Bearer "+account->oauth.accessToken);request.userAgent=smarttube::tvUserAgent(0);
            auto reply=fetch(request,2*1024*1024);smarttube::Json document;std::string error;if(!smarttube::parseJson(reply.body,document,error)) {toast("Could not load editable playlists");return;}
            auto playlists=smarttube::editablePlaylists(document);logLine("PLAYER editable_playlists HTTP=%ld count=%zu",reply.status,playlists.size());if(smoke || menuTest) return;if(playlists.empty()) {toast("No editable playlists returned for this account");return;}
            size_t chosen=0;uint32_t last=controller.read();bool showing=true;
            while(showing && !quit) {
                unsigned scale=video.width/1920;int x=980*scale;canvas->surface({video.linear,(int)video.width,(int)video.height,(int)video.width});canvas->rect(x,0,video.width-x,video.height,0xff102941);canvas->text("Add to playlist",x+30*scale,45*scale,32*scale,0xffeef6ff,850*scale);
                size_t first=chosen/9*9;for(size_t i=first;i<playlists.size() && i<first+9;++i) {int y=(160+(i-first)*80)*scale;canvas->rect(x+20*scale,y,880*scale,65*scale,i==chosen?0xff329dff:0xff173550);canvas->text((playlists[i].contains?"Saved · ":"")+playlists[i].title,x+40*scale,y+15*scale,25*scale,0xffeef6ff,840*scale);}
                canvas->text("CROSS  Add    CIRCLE  Back",x+30*scale,970*scale,23*scale,0xff9db4cc,850*scale);nativeRect(x,0,video.width-x,video.height);if(!video.frame(output.flips%2,output.flips)) {quit=true;break;}++output.flips;
                uint32_t keys=controller.read(),pressed=keys&~last;last=keys;if(pressed&PS5_PAD_BUTTON_UP && chosen) --chosen;if(pressed&PS5_PAD_BUTTON_DOWN && chosen+1<playlists.size()) ++chosen;
                if(pressed&PS5_PAD_BUTTON_OPTIONS) quit=true;if(pressed&PS5_PAD_BUTTON_CIRCLE) showing=false;
                if(pressed&PS5_PAD_BUTTON_CROSS) {if(playlists[chosen].contains) toast("This video is already in that playlist");else if(requestAction("browse/edit_playlist","\"playlistId\":"+smarttube::quoteJson(playlists[chosen].id)+",\"actions\":[{\"addedVideoId\":"+smarttube::quoteJson(item.id)+",\"action\":\"ACTION_ADD_VIDEO\"}]")) toast("Added to "+playlists[chosen].title);showing=false;}
            }
        };
        auto startDiscussion=[&]() {
            nextChatPoll=probe::benchmarkTime()+5;if(discussionJob.busy()) return;auto token=discussionToken.empty()?(chatMode?features.chatToken:features.commentsToken):discussionToken;
            if(token.empty()) {discussionAttempted=metadataReady;toast(metadataReady?(chatMode?"Chat is unavailable for this broadcast":"Comments are unavailable for this video"):"Loading video details...");return;}
            auto request=smarttube::featureRequest(chatMode?"live_chat/get_live_chat":"next","\"continuation\":"+smarttube::quoteJson(token),auth());request.userAgent=smarttube::tvUserAgent(0);discussionAttempted=discussionJob.start(request);nextChatPoll=probe::benchmarkTime()+5;
        };
        if(!strcmp(smokeMode,"--smoke-features") || !strcmp(smokeMode,"--smoke-captions") || !strcmp(smokeMode,"--smoke-live-features")) output.scriptedInput=[](unsigned frame)->uint32_t {
            if(frame==120 || frame==460 || frame==500) return PS5_PAD_BUTTON_UP;
            if(frame==121 || frame==461 || frame==501) return PS5_PAD_BUTTON_DOWN;
            if((frame>=122 && frame<=128) || (frame>=504 && frame<=510)) return PS5_PAD_BUTTON_RIGHT;
            if(frame==130 || frame==186 || frame==244 || frame==470 || frame==512) return PS5_PAD_BUTTON_CROSS;
            if(frame>=181 && frame<=184) return PS5_PAD_BUTTON_LEFT;
            if((frame>=240 && frame<=242) || (frame>=464 && frame<=467)) return PS5_PAD_BUTTON_RIGHT;
            if(frame==450) return PS5_PAD_BUTTON_CIRCLE;return 0;
        };
        if(!strcmp(smokeMode,"--smoke-seek")) output.scriptedInput=[&](unsigned frame)->uint32_t {
            if(seekTestStage<2 && frame==30) return seekTestStage?PS5_PAD_BUTTON_LEFT:PS5_PAD_BUTTON_RIGHT;
            if(seekTestStage<2 && frame==31) {++seekTestStage;return PS5_PAD_BUTTON_CROSS;}return 0;
        };
        if(!strcmp(smokeMode,"--smoke-sponsor-seek") && !sponsorFetched) {sponsorSegments={{1,3}};sponsorFetched=true;library.preferences.sponsorMute=true;logLine("SPONSORBLOCK diagnostic synthetic_segment=1 start=1 end=3");}
        auto seekTo=[&](double seconds) {if(isLive || !duration) return;requestedSeek=probe::clampSeek(seconds,duration/1000.0);output.cancelled=true;logLine("SEEK requested from=%.3f to=%.3f",output.positionMs/1000,requestedSeek);};
        output.controlsInput=[&](uint32_t pressed)->uint32_t {
            if(quit) {output.quit=output.cancelled=true;return 0;}
            if(pressed&PS5_PAD_BUTTON_OPTIONS) return pressed;
            if(discussionOpen) {
                if(pressed&PS5_PAD_BUTTON_CIRCLE) {discussionOpen=false;return 0;}
                if(pressed&PS5_PAD_BUTTON_UP && discussionOffset) --discussionOffset;if(pressed&PS5_PAD_BUTTON_DOWN && discussionOffset+1<lines.size()) ++discussionOffset;
                if(pressed&PS5_PAD_BUTTON_L1) {chatMode=!chatMode;lines.clear();discussionOffset=0;discussionToken.clear();}
                if(pressed&(PS5_PAD_BUTTON_R1|PS5_PAD_BUTTON_CROSS|PS5_PAD_BUTTON_L1)) startDiscussion();return 0;
            }
            if(!controlFocus && !isLive && duration && (pressed&(PS5_PAD_BUTTON_LEFT|PS5_PAD_BUTTON_RIGHT))) {controlFocus=true;controlRow=2;seekPreview=probe::clampSeek(output.positionMs/1000+((pressed&PS5_PAD_BUTTON_RIGHT)?10:-10),duration/1000.0);output.controlsUntil=probe::benchmarkTime()+8;return 0;}
            if(controlFocus && controlRow==2) {
                if(pressed) output.controlsUntil=probe::benchmarkTime()+8;
                if(pressed&PS5_PAD_BUTTON_CIRCLE) {controlFocus=false;seekPreview=-1;return 0;}
                if(pressed&PS5_PAD_BUTTON_UP) {controlRow=1;controlColumn=0;seekPreview=-1;return 0;}
                if(pressed&(PS5_PAD_BUTTON_LEFT|PS5_PAD_BUTTON_RIGHT)) seekPreview=probe::clampSeek((seekPreview>=0?seekPreview:output.positionMs/1000)+((pressed&PS5_PAD_BUTTON_RIGHT)?10:-10),duration/1000.0);
                if(pressed&PS5_PAD_BUTTON_CROSS) {seekTo(seekPreview>=0?seekPreview:output.positionMs/1000);return 0;}
                return 0;
            }
            if(pressed&PS5_PAD_BUTTON_UP) {if(!controlFocus) {controlFocus=true;controlRow=0;controlColumn=0;}else {controlRow=0;controlColumn=std::min(controlColumn,5);}output.controlsUntil=probe::benchmarkTime()+5;return pressed&~PS5_PAD_BUTTON_UP;}
            if(controlFocus) {
                if(pressed) output.controlsUntil=probe::benchmarkTime()+5;
                if(pressed&PS5_PAD_BUTTON_CIRCLE) {controlFocus=false;output.controlsUntil=0;return 0;}
                if(pressed&PS5_PAD_BUTTON_DOWN) {if(controlRow==0) {controlRow=1;controlColumn=std::min(controlColumn,7);}else if(!isLive && duration) {controlRow=2;seekPreview=output.positionMs/1000;return 0;}else {controlFocus=false;output.controlsUntil=0;}}
                int count=controlRow?8:6;if(pressed&PS5_PAD_BUTTON_LEFT && controlColumn) --controlColumn;if(pressed&PS5_PAD_BUTTON_RIGHT && controlColumn+1<count) ++controlColumn;
                if(!(pressed&PS5_PAD_BUTTON_CROSS)) return pressed&~(PS5_PAD_BUTTON_LEFT|PS5_PAD_BUTTON_RIGHT|PS5_PAD_BUTTON_DOWN);
                int action=controlRow?6+controlColumn:controlColumn;logLine("PLAYER control_action=%d",action);
                if(action==0) {controlFocus=false;return PS5_PAD_BUTTON_CROSS;}if(action==1) {output.previousVideo=true;return 0;}if(action==2) {output.next=true;return 0;}
                pcm->pause(true);
                if(action==3) settingsPanel(true);
                if(action==4) {library.preferences.debugMetrics=!library.preferences.debugMetrics;save();}
                if(action==5) {library.preferences.endMode=smarttube::nextEndMode(library.preferences.endMode);save();}
                if(action==6 || action==7) {int target=action==6?1:-1;std::string endpoint=features.reaction==target?(target==1?"like/removelike":"like/removedislike"):target==1?"like/like":"like/dislike";if(requestAction(endpoint,"\"target\":{\"videoId\":"+smarttube::quoteJson(item.id)+"}")) {features.reaction=features.reaction==target?0:target;toast(features.reaction==1?"Liked":features.reaction==-1?"Disliked":"Rating removed");}}
                if(action==8) {if(features.channelId.empty()) toast("Channel details are not available yet");else {openChannel=features.channelId;output.cancelled=true;}}
                if(action==9) {
                    if(features.tracks.empty() && !local) {auto request=smarttube::playerRequest(item.id,"TVHTML5","7.20260901.15.00");request.clientNumber="7";request.userAgent=smarttube::tvUserAgent(0);request.authorization=auth();auto reply=fetch(request,4*1024*1024);smarttube::Json json;std::string error;if(smarttube::parseJson(reply.body,json,error)) features.player(json);if(!features.tracks.empty()) captionAgent=request.userAgent;}
                    if(features.tracks.empty() && !local) {auto request=smarttube::playerRequest(item.id,"WEB","2.20260917.01.00");request.clientNumber="1";request.userAgent="Mozilla/5.0";auto reply=fetch(request,4*1024*1024);smarttube::Json json;std::string error;if(smarttube::parseJson(reply.body,json,error)) features.player(json);if(!features.tracks.empty()) captionAgent=request.userAgent;}
                    logLine("PLAYER captions tracks=%zu",features.tracks.size());
                    if(features.tracks.empty()) toast("No caption tracks returned for this video");else {
                        captionIndex=(captionIndex+2)%(int)(features.tracks.size()+1)-1;cues.clear();
                        if(captionIndex>=0) {auto& track=features.tracks[captionIndex];smarttube::Request request;request.httpGet=true;request.url=track.url;request.userAgent=captionAgent;auto fmt=request.url.find("&fmt=");if(fmt!=std::string::npos) {auto end=request.url.find('&',fmt+1);request.url.erase(fmt,end==std::string::npos?end:end-fmt);}request.url+="&fmt=json3";
                            auto reply=fetch(request,2*1024*1024);smarttube::Json json;std::string error;if(smarttube::parseJson(reply.body,json,error)) cues=smarttube::captionCues(json);logLine("PLAYER caption_data HTTP=%ld cues=%zu",reply.status,cues.size());toast(cues.empty()?"Captions unavailable from this source":track.label);if(cues.empty()) captionIndex=-1;
                        }else toast("Captions off");
                    }
                }
                if(action==10) choosePlaylist();
                if(action==11) {if(features.channelId.empty()) toast("Channel details are not available yet");else if(requestAction(features.subscribed?"subscription/unsubscribe":"subscription/subscribe","\"channelIds\":["+smarttube::quoteJson(features.channelId)+"],\"params\":\"\"")) {features.subscribed=!features.subscribed;toast(features.subscribed?"Subscribed":"Subscription removed");}}
                if(action==12) {discussionOpen=true;controlFocus=false;startDiscussion();}
                if(action==13) {output.pictureDim=(output.pictureDim+25)%100;toast("Picture dimming "+std::to_string(output.pictureDim)+"%");}
                pcm->pause(false);output.quit=quit;if(quit) output.cancelled=true;output.previous=controller.read();return 0;
            }return pressed;
        };
        output.controls=[&](bool visible,bool paused,unsigned frame) {
            (void)frame;probe::Document response;
            if(sponsorJob.take(response)) {smarttube::Json json;std::string error;std::vector<probe::MuteRange> ranges;if(response.error.empty() && smarttube::parseJson(response.body,json,error)) {ranges=probe::sponsorSkips(json,library.preferences.sponsorCategories,library.preferences.sponsorMinimum,duration/1000.0);sponsorMarkers=probe::sponsorSkips(json,library.preferences.sponsorCategories,library.preferences.sponsorMinimum,duration/1000.0,false);}sponsorFetched=true;sponsorSegments=std::move(ranges);if(!strcmp(smokeMode,"--smoke-sponsor-real") && !realSponsorPositioned && !sponsorSegments.empty()) {realSponsorPositioned=true;seekTo(std::max(0.0,sponsorSegments.front().start-1.0));logLine("SPONSORBLOCK diagnostic real_segment_positioning=1");}logLine("SPONSORBLOCK HTTP=%ld skip_ranges=%zu skip_supported=1",response.status,sponsorSegments.size());}

            if(library.preferences.sponsorMute && !isLive && duration && requestedSeek<0) for(const auto& segment:sponsorSegments) if(output.positionMs/1000>=segment.start && output.positionMs/1000<segment.end && segment.end<=duration/1000.0) {logLine("SPONSORBLOCK skip start=%.3f end=%.3f",segment.start,segment.end);if(library.preferences.sponsorNotices) pendingNotice="SponsorBlock skipped "+probe::playbackTime(segment.end-segment.start);if(segment.end>=duration/1000.0-0.1) {skipToEnd=true;output.cancelled=true;}else seekTo(segment.end);break;}
            if(metadataJob.take(response)) {
                if(metadataStage==0) {smarttube::Json json;std::string error;if(smarttube::parseJson(response.body,json,error)) features.next(json);metadataReady=true;metadataStage=1;
                    logLine("PLAYER metadata HTTP=%ld channel=%d captions=%zu comments=%d chat=%d",response.status,!features.channelId.empty(),features.tracks.size(),!features.commentsToken.empty(),!features.chatToken.empty());
                    if(!features.channelIcon.empty()) {smarttube::Request request;request.httpGet=true;request.url=features.channelIcon;request.userAgent=agent;metadataJob.start(request);}
                }else {channelImage=smarttube::decodeThumbnail(response.body,96,96);logLine("PLAYER channel_image HTTP=%ld ready=%d",response.status,channelImage!=nullptr);}
            }
            if(discussionJob.take(response)) {smarttube::Json json;std::string error;if(response.error.empty() && smarttube::parseJson(response.body,json,error)) {
                auto found=smarttube::discussion(json,chatMode);for(auto& line:found) {bool duplicate=false;for(const auto& old:lines) if(old.author==line.author && old.text==line.text) duplicate=true;if(!duplicate) lines.push_back(std::move(line));}
                while(lines.size()>200) lines.erase(lines.begin());auto token=smarttube::continuationIn(json);if(!token.empty()) discussionToken=token;
                if(chatMode && lines.size()>6) discussionOffset=lines.size()-6;logLine("PLAYER discussion HTTP=%ld chat=%d entries=%zu",response.status,chatMode,lines.size());
            }else toast("Could not load this discussion");}
            if(discussionOpen && metadataReady && !discussionAttempted) startDiscussion();
            if(discussionOpen && chatMode && probe::benchmarkTime()>=nextChatPoll) startDiscussion();
            unsigned scale=video.width/1920,w=video.width,h=video.height;canvas->surface({video.linear,(int)w,(int)h,(int)w});
            if(controlFocus && !paused && probe::benchmarkTime()>output.controlsUntil) controlFocus=false;
            bool show=visible || controlFocus;unsigned elapsed=(unsigned)output.positionMs;
            if(show) {
                canvas->rect(0,0,w,112*scale,0xff0c2036);canvas->text(local?"Local playback test":features.title.empty()?item.title:features.title,40*scale,18*scale,30*scale,0xffeef6ff,w-80*scale);
                if(channelImage) canvas->image(*channelImage,{40*(int)scale,61*(int)scale,40*(int)scale,40*(int)scale});
                canvas->text(features.channelName.empty()?item.channel:features.channelName,96*scale,66*scale,22*scale,0xff9db4cc,850*scale);
                char mediaInfo[180];snprintf(mediaInfo,sizeof(mediaInfo),"%dx%d · %.3f fps · %s / software (%d threads)",playbackMetrics.width,playbackMetrics.height,playbackMetrics.fps,playbackMetrics.codec,playbackMetrics.threads);canvas->text(mediaInfo,1000*scale,66*scale,20*scale,0xff9db4cc,w-1040*scale);nativeRect(0,0,w,112*scale);
                int top=h-330*scale;canvas->rect(0,top,w,330*scale,0xff0c2036);
                std::string primary[]={paused?"Play":"Pause","Previous","Next","Settings",library.preferences.debugMetrics?"Stats: On":"Stats",smarttube::endModeLabel(library.preferences.endMode)};
                std::string secondary[]={features.reaction==1?"Liked":"Like",features.reaction==-1?"Disliked":"Dislike","Channel",captionIndex>=0?"CC: On":"CC","Add to playlist",features.subscribed?"Subscribed":"Subscribe",isLive?"Chat / comments":"Comments","Dim "+std::to_string(output.pictureDim)+"%"};
                for(int row=0;row<2;++row) {int count=row?8:6,gap=1810/count;for(int col=0;col<count;++col) {int x=(50+col*gap)*scale,y=top+(row?102:24)*scale;bool active=controlFocus && controlRow==row && controlColumn==col;
                    canvas->rect(x,y,(gap-9)*scale,66*scale,active?0xff329dff:0xff19334e);
                    const char* primaryIcons[]={paused?"lb_ic_play":"lb_ic_pause","lb_ic_skip_previous","lb_ic_skip_next","icon_settings","action_video_stats",""};const char* secondaryIcons[]={"lb_ic_thumb_up","lb_ic_thumb_down","action_channel","","action_playlist_add","action_subscribe","action_chat","action_screen_off"};bool imported=row && col==2 && channelImage?false:canvas->icon(row?secondaryIcons[col]:primaryIcons[col],{x+10*(int)scale,y+16*(int)scale,34*(int)scale,34*(int)scale});
                    if(!imported && !row && col==0) {if(paused) canvas->playIcon(x+12*scale,y+19*scale,27*scale,0xffeef6ff);else {canvas->rect(x+12*scale,y+19*scale,6*scale,27*scale,0xffeef6ff);canvas->rect(x+24*scale,y+19*scale,6*scale,27*scale,0xffeef6ff);}}
                    if(!imported && !row && (col==1 || col==2)) {if(col==2) canvas->playIcon(x+12*scale,y+19*scale,27*scale,0xffeef6ff);else for(int n=0;n<27;++n) {int half=13-abs(n-13);canvas->rect(x+(40-half)*scale,y+(19+n)*scale,(half+1)*scale,scale,0xffeef6ff);}canvas->rect(x+(col==1?8:40)*scale,y+19*scale,4*scale,27*scale,0xffeef6ff);}
                    if(!imported && row && (col==0 || col==1)) {auto bar=[&](int xx,int yy,int ww,int hh) {if(col==1) yy=28-yy-hh;canvas->rect(x+(12+xx)*scale,y+(18+yy)*scale,ww*scale,hh*scale,0xffeef6ff);};bar(0,12,6,16);bar(9,9,18,19);bar(12,3,7,12);}
                    if(!imported && row && col==2) {if(channelImage) canvas->image(*channelImage,{x+12*(int)scale,y+17*(int)scale,32*(int)scale,32*(int)scale});else {canvas->rect(x+12*scale,y+17*scale,30*scale,30*scale,0xff265779);canvas->text("C",x+18*scale,y+20*scale,20*scale,0xffeef6ff,26*scale);}}
                    if(!imported && row && col==3) {canvas->rect(x+10*scale,y+21*scale,36*scale,25*scale,0xffeef6ff);canvas->text("CC",x+13*scale,y+23*scale,17*scale,0xff102941,34*scale);}
                    if(!imported && row && col==4) {for(int n=0;n<3;++n) canvas->rect(x+12*scale,y+(20+9*n)*scale,26*scale,3*scale,0xffeef6ff);canvas->rect(x+40*scale,y+35*scale,13*scale,3*scale,0xffeef6ff);canvas->rect(x+45*scale,y+30*scale,3*scale,13*scale,0xffeef6ff);}
                    if(!imported && row && col==5) {canvas->rect(x+10*scale,y+21*scale,34*scale,25*scale,0xffeef6ff);canvas->playIcon(x+21*scale,y+26*scale,15*scale,0xff19334e);}
                    if(!imported && row && col==6) {canvas->rect(x+11*scale,y+20*scale,31*scale,24*scale,0xffeef6ff);canvas->rect(x+15*scale,y+44*scale,6*scale,6*scale,0xffeef6ff);for(int n=0;n<3;++n) canvas->rect(x+(15+8*n)*scale,y+29*scale,4*scale,4*scale,0xff19334e);}
                    if(!imported && row && col==7) {for(int n=0;n<28;++n) {int half=(int)sqrt(196.0-(n-14)*(n-14));canvas->rect(x+(27-half)*scale,y+(19+n)*scale,(half+1)*scale,scale,0xffeef6ff);}}
                    canvas->text(row?secondary[col]:primary[col],x+((row || col<5)?54:12)*scale,y+20*scale,(row?20:23)*scale,0xffeef6ff,(gap-22-((row || col<5)?42:0))*scale);
                }}
                std::string details=std::to_string(sourceHeight)+"p / "+std::to_string(sourceFps)+" fps    "+(isLive?"LIVE":std::to_string(elapsed/60000)+":"+(elapsed/1000%60<10?"0":"")+std::to_string(elapsed/1000%60));
                double seconds=controlFocus && controlRow==2 && seekPreview>=0?seekPreview:elapsed/1000.0;
                if(!isLive && duration) {
                    probe::drawSeekTimeline(*canvas,w,top,scale,seconds,duration/1000.0,controlFocus && controlRow==2,sponsorMarkers);
                }else {canvas->text(details+" · Seeking unavailable",50*scale,top+185*scale,23*scale,0xff9db4cc,w-100*scale);}
                nativeRect(0,top,w,330*scale);
            }
            auto subtitle=smarttube::captionAt(cues,elapsed);if(!subtitle.empty()) {int y=show?h-390*scale:h-170*scale;canvas->rect(220*scale,y,w-440*scale,105*scale,0xff101010);canvas->text(subtitle,245*scale,y+13*scale,31*scale,0xffeef6ff,w-490*scale,2);nativeRect(220*scale,y,w-440*scale,105*scale);}
            if(discussionOpen) {int x=1200*scale;canvas->rect(x,0,w-x,h,0xff102941);canvas->text(chatMode?"Live chat":"Comments",x+25*scale,40*scale,32*scale,0xffeef6ff,650*scale);
                canvas->text(discussionJob.busy()?"Loading...":"Read only · L1 switches chat/comments",x+25*scale,100*scale,20*scale,0xff9db4cc,650*scale);
                if(lines.empty()) canvas->text(metadataReady?"No messages returned for this video":"Loading video details...",x+25*scale,210*scale,24*scale,0xffeef6ff,640*scale,3);
                for(size_t i=discussionOffset;i<lines.size() && i<discussionOffset+6;++i) {int y=(170+(i-discussionOffset)*127)*scale;canvas->text(lines[i].author,x+25*scale,y,21*scale,0xff329dff,640*scale);canvas->text(lines[i].text,x+25*scale,y+34*scale,21*scale,0xffeef6ff,640*scale,3);}
                canvas->text("UP / DOWN  Scroll   R1  More   CIRCLE  Back",x+25*scale,975*scale,18*scale,0xff9db4cc,650*scale);nativeRect(x,0,w-x,h);
            }
            if(probe::benchmarkTime()<toastUntil) {canvas->rect(290*scale,125*scale,1340*scale,76*scale,0xff102941);canvas->text(featureStatus,315*scale,143*scale,25*scale,0xffeef6ff,1290*scale);nativeRect(290*scale,125*scale,1340*scale,76*scale);}
        };
        output.debug=[&](unsigned frame) {
            if(!library.preferences.debugMetrics) return;unsigned scale=video.width/1920;
            canvas->surface({video.linear,(int)video.width,(int)video.height,(int)video.width});canvas->rect(0,0,video.width,104*scale,0xff102941);
            std::string line="Source cap "+std::to_string(library.preferences.height)+"p   Output "+std::to_string(video.width)+"x"+std::to_string(video.height)+"   Frame "+std::to_string(frame);
            canvas->text(line,24*scale,12*scale,26*scale,0xffeef6ff,video.width-48*scale);
            char metrics[160];snprintf(metrics,sizeof(metrics),"Tile avg %.2f ms   Display interval %.2f ms   >25ms %u",frame?video.tileMs/frame:0,video.intervalCount?video.intervalTotal/video.intervalCount:0,video.longIntervals);canvas->text(metrics,24*scale,58*scale,24*scale,0xff9db4cc,video.width-48*scale);
            for(unsigned y=0;y<104*scale;++y) for(unsigned x=0;x<video.width;++x) {auto& p=video.linear[(size_t)y*video.width+x];p=(p&0xff00ff00u)|((p&255u)<<16)|((p>>16)&255u);}
        };
        probe::CurlStream stream(logLine,!strcmp(smokeMode,"--smoke-reconnect")?512*1024:0),audioStream(logLine,!strcmp(smokeMode,"--smoke-reconnect")?32*1024:0);AVIOContext* io=nullptr;AVIOContext* audioIo=nullptr;probe::AdaptiveAudio* audioDecoder=nullptr;
        if(!local && started && stream.start(url,agent,isLive)) {auto* buffer=static_cast<uint8_t*>(av_malloc(65536));if(buffer) {io=avio_alloc_context(buffer,65536,0,&stream,probe::CurlStream::read,nullptr,isLive?nullptr:probe::CurlStream::seek);if(!io) av_free(buffer);}}
        if(!audioUrl.empty() && started && audioStream.start(audioUrl,agent,isLive)) {auto* buffer=static_cast<uint8_t*>(av_malloc(65536));if(buffer) {audioIo=avio_alloc_context(buffer,65536,0,&audioStream,probe::CurlStream::read,nullptr,isLive?nullptr:probe::CurlStream::seek);if(!audioIo) av_free(buffer);}}
        if(audioIo) {audioDecoder=new probe::AdaptiveAudio(audioIo,*pcm,logLine,startSeconds);if(!audioDecoder->start()) pcm->fail();}
        bool played=started && (local || io) && probe::playAv("/data/homebrew/PStube-PS5-Test/assets/av-1080.webm",presentLocal,&output,*pcm,logLine,io,limit,!audioUrl.empty(),video.width,video.height,true,isLive,&output.positionMs,startSeconds,&playbackMetrics,(int)library.preferences.decoderThreads,(int)library.preferences.scalerThreads);
        if(played && !local && !isLive && !limit && duration && output.positionMs+3000<duration) {logLine("PLAY premature_eof position_ms=%.0f expected_ms=%u",output.positionMs,duration);played=false;}
        fetchPaint={};metadataJob.cancel();discussionJob.cancel();
        if(audioDecoder) {
            if(!played) {audioDecoder->cancel();audioStream.cancel();sponsorJob.cancel();pcm->finish(false);audioDecoder->join();}
            else {audioDecoder->join();sponsorJob.cancel();pcm->finish(true);}delete audioDecoder;
        } else {sponsorJob.cancel();pcm->finish(played);}
        delete pcm;audio.sceAudioOutClose(sink.handle);if(io) {av_freep(&io->buffer);avio_context_free(&io);}if(audioIo) {av_freep(&audioIo->buffer);avio_context_free(&audioIo);}if(!audioUrl.empty()) audioStream.close();if(!local) stream.close();
        if(skipToEnd) played=true;
        logLine("APP display_result output=%ux%u frames=%llu tile_average_ms=%.3f flip_interval_average_ms=%.3f max_interval_ms=%.3f intervals_over_25ms=%u",video.width,video.height,(unsigned long long)output.flips,output.flips?video.tileMs/output.flips:0,video.intervalCount?video.intervalTotal/video.intervalCount:0,video.maxInterval,video.longIntervals);
        if(!video.mode(1920,1080)) quit=true;canvas->surface({video.linear,1920,1080,1920});
        flips=output.flips;quit=quit || output.quit;status=played?"Playback finished":output.cancelled?"Playback stopped":"Playback ended; see log";
        logLine("APP playback_end local=%d complete=%d cancelled=%d quit=%d next=%d",local,played,output.cancelled,output.quit,output.next);
        if(requestedSeek>=0 && !quit) {retainedCaption=captionIndex;retainedCues=cues;retainedDim=output.pictureDim;startSeconds=requestedSeek;status="Seeking to "+probe::playbackTime(startSeconds)+"...";screen();continue;}
        if(!played && !limit && !local && !isLive && !quit && !output.cancelled && !output.next && !output.previousVideo && output.positionMs>startSeconds*1000+500 && recoveryCount<2) {
            ++recoveryCount;retainedCaption=captionIndex;retainedCues=cues;retainedDim=output.pictureDim;startSeconds=std::max(0.0,output.positionMs/1000-0.15);
            logLine("PLAY recovery attempt=%u resume_seconds=%.3f refresh_urls=1",recoveryCount,startSeconds);status="Reconnecting video at "+probe::playbackTime(startSeconds)+"...";screen();resolvePlayback();if(!url.empty()) continue;
        }
        if(!local && !smoke && !openChannel.empty()) {reset();shelfLayout=false;browseId=openChannel;browseParams="EgZ2aWRlb3PyBgQKAjoA";query=features.channelName;receive(account->browse(browseId,"",browseParams),false);return;}
        if(!local && !smoke) {
            auto action=smarttube::afterVideo(library.preferences.endMode,!played,selected+1<items.size(),isLive);
            if(output.previousVideo && selected) {--selected;queuedPlay=true;}
            else if(output.next && selected+1<items.size()) {++selected;queuedPlay=true;}
            else if(action==smarttube::EndAction::Next) {++selected;queuedPlay=true;}
            else if(action==smarttube::EndAction::Replay) queuedPlay=true;
        }
        break;
        }
    };
    if(networkTest && global) {
        auto* capabilities=curl_version_info(CURLVERSION_NOW);logLine("NETWORK capabilities https_proxy=%d native_ipv4=1 plain_custom_dns=0",capabilities && (capabilities->features&CURL_VERSION_HTTPS_PROXY)!=0);
        for(int mode=0;mode<4 && !quit;++mode) {probe::NetworkPreferences test;test.value.dns_mode=mode;if(mode==3) strcpy(test.value.doh_url,"https://dns.google/dns-query");smarttube::Request request;request.httpGet=true;request.url="https://www.youtube.com/generate_204";request.userAgent="PStube PS5 network test";
            requestNetworkOverride=&test.value;auto response=fetch(request,1024);requestNetworkOverride=nullptr;bool ok=response.status==204 && (response.error.empty() || response.error=="HTTP 204");logLine("NETWORK diagnostic dns_mode=%d HTTP=%ld ok=%d",mode,response.status,ok);
        }
        logLine("NETWORK diagnostic_complete saved_preferences_changed=0");quit=true;
    }
    if(!smoke && !menuTest && !networkTest && global) {openMenu(smarttube::Home);sidebar=true;}
    if(smoke && global) {search("nature");smarttube::Video sample;sample.id=(!strcmp(smokeMode,"--smoke") || !strcmp(smokeMode,"--smoke-captions"))?"jNQXAC9IVRw":"aqz-KE-bpKQ";if(!strncmp(smokeMode,"--smoke-live",12)) {openMenu(smarttube::Live);if(!items.empty()) sample=items.front();}if(!strcmp(smokeMode,"--smoke-sponsor-real")) sample.id="0e3GPea1Tyg";sample.title="PS5 network playback test";play(sample,false,!strcmp(smokeMode,"--smoke-live-long")?3600:!strcmp(smokeMode,"--smoke-long")?7200:600);quit=true;logLine("APP smoke_complete");}
    auto selectedDescription=[&]() {
        probe::Document response;if(descriptionJob.take(response)) {smarttube::Json json;std::string error;std::string description;if(response.error.empty() && smarttube::parseJson(response.body,json,error)) description=smarttube::videoDescription(json);
            logLine("UI description HTTP=%ld stage=%u ready=%d bytes=%zu",response.status,descriptionStage,!description.empty(),description.size());
            if(description.empty() && descriptionStage==0 && !sidebar && selected<items.size() && items[selected].id==descriptionId) {auto request=smarttube::playerRequest(descriptionId,"WEB","2.20260917.01.00");request.clientNumber="1";request.userAgent="Mozilla/5.0";descriptionStage=1;descriptionJob.start(request);}
            else {if(descriptions.size()>=64) descriptions.erase(descriptions.begin());descriptions[descriptionId]=description.substr(0,32768);}
        }
        if(!descriptionJob.busy() && !sidebar && !editing && !accountOptions && !pairingView && !channelList && !playlistList && selected<items.size() && probe::benchmarkTime()-marqueeSince>=0.8 && !descriptions.count(items[selected].id)) {descriptionId=items[selected].id;descriptionStage=0;auto request=smarttube::featureRequest("next","\"videoId\":"+smarttube::quoteJson(descriptionId),"");request.userAgent=smarttube::tvUserAgent(0);descriptionJob.start(request);}
    };
    if(!strcmp(smokeMode,"--dearrow-test") && global) {
        reset();menu=smarttube::Home;query="DeArrow and scrolling description test";sidebar=false;
        for(const char* id:{"0e3GPea1Tyg","dQw4w9WgXc","aqz-KE-bpKQ"}) items.push_back({id,"Original video title","Public test video",std::string("https://i.ytimg.com/vi/")+id+"/hqdefault.jpg"});
        double deadline=probe::benchmarkTime()+25;while(!quit && probe::benchmarkTime()<deadline) {selectedDescription();thumbnails();thumbnailLoader.drain(images,{"0e3GPea1Tyg","dQw4w9WgXc","aqz-KE-bpKQ"},&items);screen();if(thumbnailLoader.brandingCompleted()>=3 && descriptions.count(items.front().id) && probe::benchmarkTime()-marqueeSince>6) break;}
        logLine("BETA dearrow_test requests=%u complete=%u responses=%u replacements=%u titles=%u thumbnails=%u originals_loaded=%zu description_ready=%d",thumbnailLoader.brandingRequests(),thumbnailLoader.brandingCompleted(),thumbnailLoader.brandingSuccess(),thumbnailLoader.brandingUpdates(),thumbnailLoader.brandingTitles(),thumbnailLoader.brandingImages(),images.size(),descriptions.count(items.front().id) && !descriptions[items.front().id].empty());quit=true;
    }
    if(captureTest && global) {
        reset();menu=smarttube::Home;query="Home";sidebar=false;shelfLayout=true;receive(account->browse("default","","",false),false);
        if(!shelves.empty() && shelves[0].count>3) selected=shelves[0].first+2;
        double deadline=probe::benchmarkTime()+25;unsigned ready=0;
        while(!quit && probe::benchmarkTime()<deadline) {thumbnails();std::set<std::string> visible;for(const auto& item:visibleItems()) visible.insert(item.id);thumbnailLoader.drain(images,visible,&items);screen();size_t loaded=0;for(const auto& id:visible) if(images.count(id)) ++loaded;if(loaded>=std::min((size_t)8,visible.size()) && !visible.empty()) {if(++ready>15) break;}}
        if(!quit && !images.empty()) {screen();FILE* f=fopen("/data/pstube-home-v017.ppm","wb");bool saved=f!=nullptr;if(f) {fprintf(f,"P6\n1920 1080\n255\n");unsigned char row[1920*3];for(unsigned y=0;y<1080 && saved;++y) {for(unsigned x=0;x<1920;++x) {uint32_t pixel=video.linear[(size_t)y*1920+x];row[x*3]=pixel&255;row[x*3+1]=(pixel>>8)&255;row[x*3+2]=(pixel>>16)&255;}saved=fwrite(row,1,sizeof(row),f)==sizeof(row);}if(fclose(f)) saved=false;}logLine("BETA console_capture saved=%d thumbnails=%zu guest_feed=1 profile_hidden=1",saved,images.size());}quit=true;
    }
    if(browseTest && global) {
        for(int destination:{smarttube::Home,smarttube::Music,smarttube::Gaming,smarttube::News,smarttube::Live}) {
            if(quit) break;openMenu(destination);size_t initialRows=shelves.size(),initialItems=items.size();
            if(!shelves.empty()) {selected=shelves.front().first;more();}
            for(unsigned page=0;page<3 && !quit && !continuation.empty();++page) more(true);
            logLine("BETA browse_paging menu=%d initial_rows=%zu final_rows=%zu initial_videos=%zu final_videos=%zu feed_continuation=%d",destination,initialRows,shelves.size(),initialItems,items.size(),!continuation.empty());
            if(!shelves.empty()) selected=shelves.back().first;for(unsigned n=0;n<40 && !quit;++n) {thumbnails();std::set<std::string> visible;for(const auto& item:visibleItems()) visible.insert(item.id);thumbnailLoader.drain(images,visible,&items);screen();}
        }logLine("BETA browse_paging_complete dearrow_requests=%u responses=%u title_updates=%u",thumbnailLoader.brandingRequests(),thumbnailLoader.brandingSuccess(),thumbnailLoader.brandingUpdates());quit=true;
    }
    if(menuTest && !browseTest && !captureTest && strcmp(smokeMode,"--dearrow-test") && global) {
        for(int destination:{smarttube::Home,smarttube::Subscriptions,smarttube::Channels,smarttube::AccountHistory,smarttube::Playlists,smarttube::Liked,smarttube::Music,smarttube::Gaming,smarttube::News,smarttube::Shorts,smarttube::Favorites,smarttube::LocalHistory,smarttube::Live}) {if(quit) break;openMenu(destination);screen();}
        if(!quit) search("nature");
        for(unsigned frame=0;frame<100 && !quit && !items.empty();++frame) {selected=(frame/4)%items.size();thumbnails();std::set<std::string> visible;for(const auto& item:visibleItems()) visible.insert(item.id);thumbnailLoader.drain(images,visible,&items);screen();}
        logLine("BETA thumbnail_navigation frames=100 selection=%zu loaded=%zu",selected,images.size());
        if(!quit && !account->oauth.linked()) {pairingView=true;status="Testing device pairing...";bool ready=account->begin();screen();logLine("BETA pairing_ready=%d qr_valid=%d",ready,smarttube::LoginQr(account->oauth.verificationUrl).valid);account->cancelPairing();pairingView=false;}
        logLine("BETA menu_test_complete quit=%d",quit);quit=true;
    }
    double repeatAt=0;uint64_t idleStart=milliseconds();
    while(!quit) {
        selectedDescription();std::set<std::string> visibleIds;for(const auto& item:visibleItems()) visibleIds.insert(item.id);
        unsigned completedImages=thumbnailLoader.drain(images,visibleIds,&items);if(completedImages) logLine("UI thumbnails_completed=%u cache=%zu selection=%zu",completedImages,images.size(),selected);
        if(queuedPlay && selected<items.size()) {queuedPlay=false;play(items[selected],false,0);continue;}
        uint32_t buttons=controller.read(),pressed=buttons&~previous;previous=buttons;double now=millisecondsPrecise()/1000;
        uint32_t directions=PS5_PAD_BUTTON_UP|PS5_PAD_BUTTON_DOWN|PS5_PAD_BUTTON_LEFT|PS5_PAD_BUTTON_RIGHT;
        if(pressed&directions) repeatAt=now+0.4;else if((buttons&directions) && now>=repeatAt) {pressed|=buttons&directions;repeatAt=now+0.15;}
        if(pressed&PS5_PAD_BUTTON_OPTIONS) break;
        if(pairingView) {
            if(pressed&PS5_PAD_BUTTON_CIRCLE) {account->cancelPairing();pairingView=false;accountOptions=true;if(account->oauth.linked()) account->loadProfile();}
            else if(pressed&PS5_PAD_BUTTON_CROSS) account->begin();
            if(account->pairing && account->poll()) {pairingView=false;accountOptions=true;status="Account linked";}
        } else if(accountOptions) {
            if((pressed&PS5_PAD_BUTTON_UP) && accountRow) --accountRow;if((pressed&PS5_PAD_BUTTON_DOWN) && accountRow<5) ++accountRow;
            if(pressed&PS5_PAD_BUTTON_CIRCLE) accountOptions=false;
            if(pressed&PS5_PAD_BUTTON_CROSS) {
                if(accountRow==0) {pairingView=true;accountOptions=false;status="Preparing Google sign-in...";account->begin();}
                if(accountRow==1) {account->slots.cycle(account->oauth);account->loadProfile();reset();}
                if(accountRow==2) {account->slots.guest(account->oauth);account->profile={};account->avatar.reset();reset();}
                if(accountRow==3) account->oauth.status=account->slots.setDefault()?"Startup default saved":"Could not save default";
                if(accountRow==4) account->oauth.status=account->slots.useLastSelected()?"Restoring last account at startup":"Could not save default";
                if(accountRow==5) {account->slots.forget(account->oauth);account->profile={};account->avatar.reset();reset();}
            }
        } else if(editing) {
            if(pressed&PS5_PAD_BUTTON_LEFT) keyboard.move(-1,0);if(pressed&PS5_PAD_BUTTON_RIGHT) keyboard.move(1,0);
            if(pressed&PS5_PAD_BUTTON_UP) keyboard.move(0,-1);if(pressed&PS5_PAD_BUTTON_DOWN) keyboard.move(0,1);
            if(pressed&PS5_PAD_BUTTON_L1) keyboard.erase();if(pressed&PS5_PAD_BUTTON_SQUARE) keyboard.upper=!keyboard.upper;if(pressed&PS5_PAD_BUTTON_CIRCLE) editing=false;
            if(((pressed&PS5_PAD_BUTTON_CROSS) && keyboard.select()) || ((pressed&PS5_PAD_BUTTON_R1) && !keyboard.value.empty())) {editing=false;search(keyboard.value);}
        } else {
            if(pressed&PS5_PAD_BUTTON_TRIANGLE) {editing=true;keyboard=smarttube::Keyboard{};}
            if(pressed&PS5_PAD_BUTTON_L2) {settingsPanel(false);canvas->surface({video.linear,1920,1080,1920});previous=controller.read();}
            if(pressed&PS5_PAD_BUTTON_CIRCLE) sidebar=!sidebar;
            if(sidebar) {
                if((pressed&PS5_PAD_BUTTON_UP) && menu>=0) --menu;if((pressed&PS5_PAD_BUTTON_DOWN) && menu<smarttube::MenuCount-1) ++menu;
                if(pressed&PS5_PAD_BUTTON_RIGHT) sidebar=false;
                if(pressed&PS5_PAD_BUTTON_CROSS) {if(menu==-1) {accountOptions=true;accountRow=0;}else openMenu(menu);previous=controller.read();}
            } else if(!items.empty()) {
                int columns=channelList?1:smarttube::ResultColumns;
                if(!shelves.empty()) {
                    if((pressed&PS5_PAD_BUTTON_LEFT) && selected==shelves[smarttube::selectedShelf(shelves,selected)].first) sidebar=true;
                    selected=smarttube::moveShelf(shelves,selected,(pressed&PS5_PAD_BUTTON_LEFT)?-1:(pressed&PS5_PAD_BUTTON_RIGHT)?1:0,(pressed&PS5_PAD_BUTTON_UP)?-1:(pressed&PS5_PAD_BUTTON_DOWN)?1:0);
                }else {
                if((pressed&PS5_PAD_BUTTON_LEFT) && selected%columns==0) sidebar=true;
                else if((pressed&PS5_PAD_BUTTON_LEFT) && selected) --selected;
                if((pressed&PS5_PAD_BUTTON_RIGHT) && selected+1<items.size()) ++selected;
                if((pressed&PS5_PAD_BUTTON_UP) && selected>=(size_t)columns) selected-=columns;
                if((pressed&PS5_PAD_BUTTON_DOWN) && selected+(size_t)columns<items.size()) selected+=columns;
                }
                if((pressed&PS5_PAD_BUTTON_SQUARE) && !channelList && !playlistList) {library.toggleFavorite(items[selected]);save();status=library.favorite(items[selected].id)?"Added to favorites":"Removed from favorites";}
                if(pressed&PS5_PAD_BUTTON_CROSS) {
                    if(channelList || playlistList) {auto item=items[selected];bool channel=channelList;reset();shelfLayout=false;browseId=item.browseId;browseParams=channel?(item.browseParams.empty()?"EgZ2aWRlb3PyBgQKAjoA":item.browseParams):"";query=item.title;receive(account->browse(browseId,"",browseParams),false);}
                    else play(items[selected],false,0);
                }
                if(pressed&PS5_PAD_BUTTON_R1) more();
                if(pressed&PS5_PAD_BUTTON_DOWN) {if(shelves.empty() && selected+12>=items.size()) more();else if(!shelves.empty() && smarttube::selectedShelf(shelves,selected)+2>=shelves.size()) more(true);}
                if(!shelves.empty() && (pressed&PS5_PAD_BUTTON_RIGHT)) {auto& row=shelves[smarttube::selectedShelf(shelves,selected)];if(selected+3>=row.first+row.count) more();}
            } else if(pressed&PS5_PAD_BUTTON_LEFT) sidebar=true;
        }
        if(!editing && !accountOptions && !pairingView && !items.empty()) thumbnails();
        if(pressed) idleStart=milliseconds();if(milliseconds()-idleStart>1800000) break;screen();
    }
    descriptionJob.cancel();thumbnailLoader.shutdown();account->cancelPairing();logLine("APP exited UI_flips=%llu",(unsigned long long)flips);delete canvas;if(net) dlclose(net);
}
