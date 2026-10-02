// Copyright (c) 2026 StockiestThree9
#include "platform.hpp"
#include <SDL_image.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#ifdef __SWITCH__
#include <switch.h>
extern "C" {
#include <pulsar.h>
}
#endif
namespace zse {
namespace fs=std::filesystem;
struct Platform::Impl {
    std::map<int,TTF_Font*> fonts;
    uint32_t lastHeld=0;
    uint64_t repeatAt=0;
#ifdef __SWITCH__
    PadState pad{};
    PlFontData sharedFont{};
    bool account=false,fontService=false,system=false,audio=false;
    std::array<PLSR_PlayerSoundId,4> sounds{};
#endif
};
#ifdef __SWITCH__
static AccountUid uid(const Profile& p){return {{p.low,p.high}};}
static void check(Result rc,const char* action){if(R_FAILED(rc)){char text[128];snprintf(text,sizeof(text),"%s (code %08X)",action,unsigned(rc));throw Error(text);}}
struct Mount {
    Mount(const Profile& p,bool write=false){check(write?fsdevMountSaveData("zsave",BotwId,uid(p)):fsdevMountSaveDataReadOnly("zsave",BotwId,uid(p)),"Could not open this user's BOTW save");}
    ~Mount(){fsdevUnmountDevice("zsave");}
};
#endif
Platform::Platform():impl_(new Impl) {
    fs::create_directories(dataPath());
    loadSettings();
#ifdef __SWITCH__
    romfsInit();
    impl_->account=R_SUCCEEDED(accountInitialize(AccountServiceType_System));
    impl_->fontService=R_SUCCEEDED(plInitialize(PlServiceType_User));
    if(impl_->fontService)check(plGetSharedFontByType(&impl_->sharedFont,PlSharedFontType_Standard),"Could not load the system font");
    impl_->system=R_SUCCEEDED(setsysInitialize());
    padConfigureInput(1,HidNpadStyleSet_NpadStandard);padInitializeDefault(&impl_->pad);hidInitializeTouchScreen();
    appletSetScreenShotPermission(AppletScreenShotPermission_Enable);
    impl_->audio=PLSR_RC_SUCCEEDED(plsrPlayerInit());
    if(impl_->audio && R_SUCCEEDED(romfsMountDataStorageFromProgram(0x0100000000001000ULL,"zsound"))) {
        PLSR_BFSAR ar{};
        if(PLSR_RC_SUCCEEDED(plsrBFSAROpen("zsound:/sound/qlaunch.bfsar",&ar))) {
            const char* names[]={"SeBtnFocus","SeBtnDecide","SeFooterDecideFinish","SeKeyError"};
            for(size_t i=0;i<4;++i)if(PLSR_RC_SUCCEEDED(plsrPlayerLoadSoundByName(&ar,names[i],&impl_->sounds[i])))plsrPlayerSetVolume(impl_->sounds[i],0.65f);
            plsrBFSARClose(&ar);
            nativeSounds=impl_->sounds[0]&&impl_->sounds[1]&&impl_->sounds[2];
        }
        romfsUnmount("zsound");
    }
#endif
}
Platform::~Platform(){
    for(auto& f:impl_->fonts)TTF_CloseFont(f.second);
#ifdef __SWITCH__
    if(impl_->audio){for(auto s:impl_->sounds)if(s)plsrPlayerFree(s);plsrPlayerExit();}
    if(impl_->fontService)plExit();if(impl_->account)accountExit();if(impl_->system)setsysExit();romfsExit();
#endif
}
std::string Platform::dataPath() const {
#ifdef __SWITCH__
    return "sdmc:/switch/zelda-save-editor";
#else
    return "host-data";
#endif
}
bool Platform::dark() const {
    if(settings.theme!=Theme::Auto)return settings.theme==Theme::Dark||settings.theme==Theme::Oled;
#ifdef __SWITCH__
    ColorSetId id=ColorSetId_Light;if(impl_->system)setsysGetColorSetId(&id);return id==ColorSetId_Dark;
#else
    return false;
#endif
}
bool Platform::appletMode() const {
#ifdef __SWITCH__
    return appletGetAppletType()!=AppletType_Application&&appletGetAppletType()!=AppletType_SystemApplication;
#else
    return false;
#endif
}
TTF_Font* Platform::font(int size){
    auto it=impl_->fonts.find(size);if(it!=impl_->fonts.end())return it->second;
    TTF_Font* result=nullptr;
#ifdef __SWITCH__
    if(impl_->sharedFont.address)result=TTF_OpenFontRW(SDL_RWFromConstMem(impl_->sharedFont.address,impl_->sharedFont.size),1,size);
#else
    result=TTF_OpenFont("C:/Windows/Fonts/segoeui.ttf",size);
#endif
    if(!result)throw Error(std::string("Could not open font: ")+TTF_GetError());
    impl_->fonts.emplace(size,result);return result;
}
void Platform::cue(Cue c){
    if(!settings.sound)return;
#ifdef __SWITCH__
    auto s=impl_->sounds[size_t(c)];if(s)plsrPlayerPlay(s);
#else
    (void)c;
#endif
}
Input Platform::poll(){
    Input in;uint32_t held=0;SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type==SDL_QUIT)in.quit=true;
        if(e.type==SDL_FINGERDOWN){in.touchX=int(e.tfinger.x*1280);in.touchY=int(e.tfinger.y*720);}
#ifndef __SWITCH__
        if(e.type==SDL_MOUSEBUTTONDOWN){in.touchX=e.button.x;in.touchY=e.button.y;}
        if(e.type==SDL_KEYDOWN&&!e.key.repeat){switch(e.key.keysym.sym){case SDLK_RETURN:in.down|=A;break;case SDLK_ESCAPE:in.down|=B;break;case SDLK_x:in.down|=X;break;case SDLK_y:in.down|=Y;break;case SDLK_q:in.down|=L;break;case SDLK_e:in.down|=R;break;case SDLK_s:in.down|=Plus;break;case SDLK_z:in.down|=ZL;break;case SDLK_r:in.down|=ZR;break;case SDLK_F10:in.quit=true;break;}}
#endif
    }
#ifdef __SWITCH__
    if(!appletMainLoop())in.quit=true;
    padUpdate(&impl_->pad);uint64_t raw=padGetButtons(&impl_->pad),down=padGetButtonsDown(&impl_->pad);
    const std::pair<uint64_t,uint32_t> buttons[]={{HidNpadButton_Up|HidNpadButton_StickLUp,Up},{HidNpadButton_Down|HidNpadButton_StickLDown,Down},{HidNpadButton_Left|HidNpadButton_StickLLeft,Left},{HidNpadButton_Right|HidNpadButton_StickLRight,Right},{HidNpadButton_A,A},{HidNpadButton_B,B},{HidNpadButton_X,X},{HidNpadButton_Y,Y},{HidNpadButton_L,L},{HidNpadButton_R,R},{HidNpadButton_Plus,Plus},{HidNpadButton_Minus,Minus},{HidNpadButton_ZL,ZL},{HidNpadButton_ZR,ZR}};
    for(auto& b:buttons){if(raw&b.first)held|=b.second;if(down&b.first)in.down|=b.second;}
#else
    auto keys=SDL_GetKeyboardState(nullptr);if(keys[SDL_SCANCODE_UP])held|=Up;if(keys[SDL_SCANCODE_DOWN])held|=Down;if(keys[SDL_SCANCODE_LEFT])held|=Left;if(keys[SDL_SCANCODE_RIGHT])held|=Right;
#endif
    uint32_t dirs=held&(Up|Down|Left|Right);uint64_t now=SDL_GetTicks64();
    if(dirs!=impl_->lastHeld){in.down|=dirs;impl_->lastHeld=dirs;impl_->repeatAt=now+350;}
    else if(dirs&&now>=impl_->repeatAt){in.down|=dirs;impl_->repeatAt=now+95;}
    return in;
}
void Platform::loadSettings(){
    std::ifstream in(dataPath()+"/settings.ini");std::string key;int v;
    while(in>>key>>v){if(key=="theme"&&v>=0&&v<=3)settings.theme=Theme(v);if(key=="sounds")settings.sound=v!=0;}
}
void Platform::saveSettings(){
    std::string text="theme "+std::to_string(int(settings.theme))+"\nsounds "+std::to_string(settings.sound)+"\n";
    auto path=dataPath()+"/settings.ini";writeFile(path+".tmp",Bytes(text.begin(),text.end()));
    // Settings can be recreated; keep a previous copy if a replacement is interrupted.
    std::error_code ec;fs::remove(path+".old",ec);if(fs::exists(path))fs::rename(path,path+".old");fs::rename(path+".tmp",path);
}
std::optional<std::string> Platform::keyboard(const std::string& title,const std::string& initial,size_t max,bool numeric,bool decimal){
#ifdef __SWITCH__
    SwkbdConfig config{};check(swkbdCreate(&config,0),"Could not open the keyboard");
    swkbdConfigMakePresetDefault(&config);swkbdConfigSetType(&config,numeric?SwkbdType_NumPad:SwkbdType_Normal);
    if(numeric){swkbdConfigSetLeftOptionalSymbolKey(&config,"-");if(decimal)swkbdConfigSetRightOptionalSymbolKey(&config,".");}
    swkbdConfigSetHeaderText(&config,title.c_str());swkbdConfigSetInitialText(&config,initial.c_str());swkbdConfigSetStringLenMax(&config,std::min(size_t(255),max));
    char out[1024]{};Result rc=swkbdShow(&config,out,sizeof(out));swkbdClose(&config);if(R_FAILED(rc))return {};return std::string(out);
#else
    (void)title;(void)initial;(void)max;(void)numeric;(void)decimal;return {};
#endif
}
void Platform::log(const std::string& text) const { std::ofstream(dataPath()+"/log.txt",std::ios::app)<<text<<'\n'; }
std::vector<Profile> Platform::profiles(){
    std::vector<Profile> result;
#ifdef __SWITCH__
    if(!impl_->account)throw Error("The profile service is unavailable. Launch with R held over a game.");
    AccountUid users[8]{};s32 count=0;check(accountListAllUsers(users,8,&count),"Could not list profiles");
    for(s32 i=0;i<count;++i){AccountProfile prof{};if(R_FAILED(accountGetProfile(&prof,users[i])))continue;
        AccountProfileBase base{};Result rc=accountProfileGet(&prof,nullptr,&base);
        if(R_SUCCEEDED(rc)){Profile p{users[i].uid[0],users[i].uid[1],std::string(base.nickname,std::find(base.nickname,base.nickname+32,'\0')),{}};u32 size=0;
            if(R_SUCCEEDED(accountProfileGetImageSize(&prof,&size))&&size<1024*1024){p.image.resize(size);u32 actual=0;if(R_FAILED(accountProfileLoadImage(&prof,p.image.data(),size,&actual)))p.image.clear();else p.image.resize(actual);}
            result.push_back(std::move(p));}
        accountProfileClose(&prof);
    }
#else
    result.push_back({1,2,"StockiestThree9",{}});
#endif
    return result;
}
std::vector<Slot> Platform::slots(const Profile& p){
#ifdef __SWITCH__
    Mount mount(p);std::string root="zsave:/";
#else
    (void)p;std::string root=dataPath()+"/saves/";
#endif
    std::vector<Slot> result;std::error_code ec;
    if(!fs::exists(root,ec))return result;
    if(profileSwapPending(root))throw Error("Resume the interrupted profile swap before opening saves.");
    size_t visited=0;
    std::function<void(const fs::path&,const std::string&,int)> scan;
    scan=[&](const fs::path& directory,const std::string& relative,int depth){
        if(++visited>512)throw Error("The save contains too many folders to inspect.");
        for(auto& entry:fs::directory_iterator(directory)) {
            if(entry.is_symlink()||!entry.is_directory())continue;
            auto name=entry.path().filename().string();auto folder=relative.empty()?name:relative+"/"+name;
            if(name=="album"||name=="pict_book"||name=="tracker"||name=="zse-profile-swap")continue;
            if(!fs::is_regular_file(entry.path()/"game_data.sav")){if(depth<4)scan(entry.path(),folder,depth+1);continue;}
        Slot slot;slot.folder=folder;
        // Observed on transferred BOTW 1.9 data: root slots are Profile 1, ch_01 contains Profile 2.
        // Keep unfamiliar layouts editable, but do not guess their profile or mode.
        bool numbered=!name.empty()&&name.size()<3&&name.find_first_not_of("0123456789")==std::string::npos;
        int id=numbered?std::stoi(name):-1;
        slot.gameProfile=relative.empty()&&id>=0&&id<=7?1:relative=="ch_01"&&id>=0&&id<=7?2:0;
        slot.master=slot.gameProfile&&(id==6||id==7);
        slot.automatic=slot.gameProfile&&((id>=1&&id<=5)||id==7);
        slot.label=slot.gameProfile?(slot.automatic?"Autosave":"Manual save"):"Save · "+folder;
        try{slot.image=readFile((entry.path()/"caption.jpg").string(),4*1024*1024);}catch(const Error&){}
        try{
            Save caption(readFile((entry.path()/"caption.sav").string()));
            if(caption.count("SaveLocationName"))slot.location=caption.string(hashName("SaveLocationName"),256);
            if(caption.count("SaveDistrictName"))slot.district=caption.string(hashName("SaveDistrictName"),256);
            if(caption.count("IsSaveByAuto"))slot.automatic=caption.word(hashName("IsSaveByAuto"))!=0;
            if(slot.gameProfile)slot.label=slot.automatic?"Autosave":"Manual save";
            if(caption.count("LastSaveTime_Lower")&&caption.count("LastSaveTime_Upper"))slot.timestamp=(uint64_t(caption.word(hashName("LastSaveTime_Upper")))<<32)|caption.word(hashName("LastSaveTime_Lower"));
            if(slot.timestamp>=946684800&&slot.timestamp<=4102444800ULL){std::time_t t=std::time_t(slot.timestamp);char stamp[64];auto time=std::localtime(&t);if(time&&std::strftime(stamp,sizeof(stamp),"%b %d, %Y · %I:%M %p",time))slot.date=stamp;}
        }catch(const Error&){}
        if(slot.master)slot.label+=" · Master Mode";
        if(slot.date.empty())slot.date="Date unavailable";
        log("Found "+slot.identity()+" at "+slot.folder);
        result.push_back(std::move(slot));
        if(result.size()>64)throw Error("The save contains more than 64 save slots.");
    }};
    scan(root,"",0);
    std::sort(result.begin(),result.end(),[](auto& a,auto& b){if(a.timestamp!=b.timestamp)return a.timestamp>b.timestamp;if(a.gameProfile!=b.gameProfile)return a.gameProfile<b.gameProfile;return a.folder<b.folder;});return result;
}
static void validateSlotPath(const Slot& slot){
    // Slot paths always remain relative to this user's mounted save, including nested profiles.
    if(slot.folder.empty()||slot.folder.size()>240)throw Error("Invalid save location.");
    size_t start=0;
    while(start<slot.folder.size()){
        size_t end=slot.folder.find('/',start);auto part=slot.folder.substr(start,end==std::string::npos?end:end-start);
        if(part.empty()||part=="."||part==".."||part.find_first_of("\\:")!=std::string::npos)throw Error("Invalid save location.");
        if(end==std::string::npos)return;start=end+1;
    }
    throw Error("Invalid save location.");
}
Save Platform::open(const Profile& p,const Slot& slot){
    validateSlotPath(slot);
#ifdef __SWITCH__
    Mount mount(p);if(profileSwapPending("zsave:/"))throw Error("Resume the interrupted profile swap first.");return Save(readFile("zsave:/"+slot.folder+"/game_data.sav"));
#else
    (void)p;if(profileSwapPending(dataPath()+"/saves/"))throw Error("Resume the interrupted profile swap first.");return Save(readFile(dataPath()+"/saves/"+slot.folder+"/game_data.sav"));
#endif
}
std::string Platform::commit(const Profile& p,const Slot& slot,Save& save){
    validateSlotPath(slot);
    if(!save.dirty())throw Error("There are no changes to save.");
#ifdef __SWITCH__
    Mount mount(p,true);std::string root="zsave:/"+slot.folder;
    if(profileSwapPending("zsave:/"))throw Error("Resume the interrupted profile swap first.");
#else
    std::string root=dataPath()+"/saves/"+slot.folder;
    if(profileSwapPending(dataPath()+"/saves/"))throw Error("Resume the interrupted profile swap first.");
#endif
    std::string target=root+"/game_data.sav";
    if(readFile(target)!=save.original())throw Error("This save changed after you opened it. Reopen it before saving.");
    auto now=std::time(nullptr);std::tm utc=*std::gmtime(&now);char stamp[32];std::strftime(stamp,sizeof(stamp),"%Y%m%d-%H%M%S",&utc);
    std::ostringstream profile;profile<<std::hex<<std::setw(16)<<std::setfill('0')<<p.high<<std::setw(16)<<p.low;
    // Encode separators so a nested profile has one unique backup directory name.
    std::string token;const char* hex="0123456789ABCDEF";
    for(unsigned char c:slot.folder){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_')token+=char(c);else{token+='%';token+=hex[c>>4];token+=hex[c&15];}}
    const std::string backupBase=dataPath()+"/backups/"+profile.str()+"/"+stamp+"-profile"+std::to_string(slot.gameProfile)+"-slot"+token;
    std::string backup=backupBase;
    for(int i=1;fs::exists(backup);++i)backup=backupBase+"-"+std::to_string(i);
    fs::create_directories(backup);
    for(auto& entry:fs::directory_iterator(root))if(entry.is_regular_file()){
        auto bytes=readFile(entry.path().string());auto dest=backup+"/"+entry.path().filename().string();writeFile(dest,bytes);if(readFile(dest)!=bytes)throw Error("Backup verification failed. The original save has not been changed.");
    }
    if(readFile(backup+"/game_data.sav")!=save.original())throw Error("Backup verification failed.");
    std::string origin=slot.identity()+"\nOriginal folder: "+slot.folder+"\nConsole user: "+p.name+"\n";
    writeFile(backup+"/zse-backup-origin.txt",Bytes(origin.begin(),origin.end()));
    try {
        writeFile(target,save.bytes());
        if(readFile(target)!=save.bytes())throw Error("Save verification failed.");
#ifdef __SWITCH__
        check(fsdevCommitDevice("zsave"),"The Switch could not commit the save");
#endif
    } catch(const std::exception& problem) {
        std::string reason=problem.what();
        try{writeFile(target,save.original());
#ifdef __SWITCH__
            check(fsdevCommitDevice("zsave"),"Restore failed");
#endif
            if(readFile(target)!=save.original())throw Error("Restore verification failed");
        }catch(...){throw Error(reason+". Restore also failed. Recover from "+backup);}
        throw Error(reason+". The original was restored. Backup: "+backup);
    }
    save.accepted();log("Saved "+slot.identity()+" at "+slot.folder+". Backup: "+backup);return backup;
}
SwapInfo Platform::swapInfo(const Profile& p){
#ifdef __SWITCH__
    Mount mount(p);return inspectProfiles("zsave:/");
#else
    (void)p;return inspectProfiles(dataPath()+"/saves/");
#endif
}
std::string Platform::swapProfiles(const Profile& p,bool resume){
#ifdef __SWITCH__
    Mount mount(p,true);fs::path root="zsave:/";auto commit=[] {check(fsdevCommitDevice("zsave"),"Could not commit the profile swap. Reopen the editor to resume");};
#else
    fs::path root=dataPath()+"/saves/";auto commit=[]{};
#endif
    if(resume)return continueProfileSwap(root,commit);
    std::ostringstream user;user<<std::hex<<std::setw(16)<<std::setfill('0')<<p.high<<std::setw(16)<<p.low;
    auto now=std::time(nullptr);std::tm utc=*std::gmtime(&now);char stamp[32];std::strftime(stamp,sizeof(stamp),"%Y%m%d-%H%M%S",&utc);
    std::string base=dataPath()+"/backups/"+user.str()+"/"+stamp+"-profile-swap",backup=base;
    for(int i=1;fs::exists(backup);++i)backup=base+"-"+std::to_string(i);
    auto result=exchangeProfiles(root,backup,commit);log("Exchanged normal BOTW saves and normal Hero's Path. Full backup: "+result);return result;
}
void Platform::loadMetadata(Metadata& data){
    const auto cache=dataPath()+"/cache";fs::create_directories(cache);
#ifdef __SWITCH__
    bool mounted=false,raw=false;FsFileSystem game{};
    Result rc=fsOpenFileSystemWithPatch(&game,BotwId,FsFileSystemType_ContentData);
    if(R_SUCCEEDED(rc))mounted=fsdevMountDevice("zgame",game)>=0;
    if(!mounted){rc=romfsMountDataStorageFromProgram(BotwId,"zgame");mounted=R_SUCCEEDED(rc);raw=mounted;}
    if(!mounted){rc=romfsMountFromCurrentProcess("zgame");mounted=R_SUCCEEDED(rc);raw=mounted;}
    if(mounted){
        try{auto pack=readFile("zgame:/Pack/Bootup.pack");data.loadGameData(pack);writeFile(cache+"/Bootup.pack",pack);
            try{auto language=readFile("zgame:/Pack/Bootup_USen.pack");data.loadMessages(language);writeFile(cache+"/Bootup_USen.pack",language);}catch(const std::exception& e){log(e.what());}
        }catch(const std::exception& e){log(e.what());}
        if(raw)romfsUnmount("zgame");else fsdevUnmountDevice("zgame");
    }else {char error[64];snprintf(error,sizeof(error),"Game resource mount: %08X",unsigned(rc));log(error);}
#endif
    if(data.fields.empty())try{data.loadGameData(readFile(cache+"/Bootup.pack"));}catch(const std::exception& e){log(e.what());}
    if(data.names.empty())try{data.loadMessages(readFile(cache+"/Bootup_USen.pack"));}catch(const std::exception& e){log(e.what());}
    resourceStatus=data.fields.empty()?"Game dictionary unavailable. Launch with R held over BOTW.":std::to_string(data.fields.size())+" game fields loaded";
    log(resourceStatus);
}
}
