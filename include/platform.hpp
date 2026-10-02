// Copyright (c) 2026 StockiestThree9
#pragma once
#include "resources.hpp"
#include "profile_swap.hpp"
#include <array>
#include <optional>
#include <SDL.h>
#include <SDL_ttf.h>

namespace zse {
constexpr uint64_t BotwId=0x01007ef00011e000ULL;
struct Profile { uint64_t low=0,high=0; std::string name; Bytes image; };
struct Slot {
    std::string folder,label;
    Bytes image;
    std::string location,district,date;
    uint64_t timestamp=0;
    bool automatic=false;
    int gameProfile=0;
    bool master=false;
    std::string profileLabel() const {return gameProfile?"Profile "+std::to_string(gameProfile):"Additional save";}
    std::string identity() const {return profileLabel()+" · "+label;}
};
enum class Cue { Focus,Confirm,Back,Error };
enum class Theme { Auto,Light,Dark,Oled };
struct Settings { Theme theme=Theme::Auto;bool sound=true; };
enum Button : uint32_t { Up=1,Down=2,Left=4,Right=8,A=16,B=32,X=64,Y=128,L=256,R=512,Plus=1024,Minus=2048,ZL=4096,ZR=8192 };
struct Input { uint32_t down=0;bool quit=false;int touchX=-1,touchY=-1; };
class Platform {
public:
    Platform();
    ~Platform();
    Settings settings;
    std::string resourceStatus;
    bool nativeSounds=false;
    bool dark() const;
    bool appletMode() const;
    TTF_Font* font(int size);
    Input poll();
    void cue(Cue cue);
    void loadSettings();
    void saveSettings();
    std::optional<std::string> keyboard(const std::string& title,const std::string& initial,size_t max,bool numeric=false,bool decimal=false);
    std::vector<Profile> profiles();
    std::vector<Slot> slots(const Profile& profile);
    Save open(const Profile& profile,const Slot& slot);
    std::string commit(const Profile& profile,const Slot& slot,Save& save);
    SwapInfo swapInfo(const Profile& profile);
    std::string swapProfiles(const Profile& profile,bool resume=false);
    void loadMetadata(Metadata& data);
    std::string dataPath() const;
    void log(const std::string& text) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
