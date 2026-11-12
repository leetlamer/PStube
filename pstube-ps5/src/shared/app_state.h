#pragma once
#include "core.h"
namespace smarttube {
constexpr int SupportedVideoHeight=2160;
int defaultQuality(int height);
int cycleDefaultQuality(int height,int direction);
enum class EndMode {AutoNext=0,Loop=1,Stop=2};
inline EndMode nextEndMode(EndMode mode) {return static_cast<EndMode>((static_cast<int>(mode)+1)%3);}
inline const char* endModeLabel(EndMode mode) {return mode==EndMode::Loop?"Loop video":mode==EndMode::Stop?"Stop at end":"Auto next";}
enum class EndAction {Stop,Next,Replay};
inline EndAction afterVideo(EndMode mode,bool failed,bool hasNext,bool live=false) {
    if(failed) return EndAction::Stop;
    if(mode==EndMode::Loop && !live) return EndAction::Replay;
    return mode==EndMode::AutoNext && hasNext?EndAction::Next:EndAction::Stop;
}
struct Preferences {
    int height=1080;
    int maxFps=60;
    unsigned decoderThreads=4,scalerThreads=2; // Per-video experiments; baseline preserved.
    bool softwareCodecs=true,preferAv1=false,autoRecovery=true;std::string audioLanguage="original";
    unsigned seconds=0; // zero selects the complete indexed video
    EndMode endMode=EndMode::AutoNext;
    bool native4k=false,debugMetrics=false;
    bool sponsorMute=false,dearrow=false; // Legacy flag now enables sponsor skipping.
    bool dearrowTitles=true,dearrowThumbnails=true;
    unsigned sponsorCategories=1,sponsorMinimum=0;bool sponsorNotices=true;
    bool fastStart=false; // Fragmented file experiment is disabled for native AVPlayer.
    std::string query; // Search is session-only; Home starts empty.
};
// A playback override must never overwrite the saved preference for later videos.
struct PlaybackQuality {
    int height=SupportedVideoHeight;unsigned poorWindows=0;
    void begin(const Preferences& preferences,int overrideHeight=0);
    int observe(int sourceHeight,unsigned sourceFps,unsigned delivered,double seconds,bool eligible,bool late,bool starved=false);
};
struct Library {
    Preferences preferences;
    std::vector<Video> favorites,history;
    bool load(const char* path);
    bool save(const char* path) const;
    bool favorite(const std::string& id) const;
    void toggleFavorite(const Video& video);
    void watched(const Video& video);
    void remember(const std::string& id,unsigned milliseconds);
    unsigned resume(const std::string& id) const;
};
struct Keyboard {
    std::string value;
    int row=0,column=0;
    bool upper=false;
    void move(int dx,int dy);
    std::string label(int r,int c) const;
    bool select(); // True submits a nonempty query.
    void erase();
};
constexpr int ResultColumns=4,ResultRows=3,ChannelRows=6;
inline size_t resultFirst(size_t selected,bool channels=false) {if(channels) return selected>=ChannelRows?selected-ChannelRows+1:0;const size_t row=selected/ResultColumns;return row>=ResultRows?(row-ResultRows+1)*ResultColumns:0;}
constexpr int QualityCount=8;
int qualityHeight(int index);
int qualityIndex(int height);
int cycleQuality(int height,int direction);
std::string qualityLabel(int height);
size_t videoBudget(int height);
size_t gridMove(size_t selected,size_t count,int dx,int dy,int columns=3);
}
