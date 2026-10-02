// Copyright (c) 2026 StockiestThree9
#include "platform.hpp"
#include <filesystem>
#include <iostream>
#include <algorithm>
using namespace zse;
static void append(Bytes& b,uint32_t v){for(int i=0;i<4;++i)b.push_back(uint8_t(v>>(i*8)));}
static Bytes document(){Bytes b;for(auto v:{0x471eu,0xffffffffu,1u,hashName("CurrentRupee"),10u,0xffffffffu})append(b,v);return b;}
static Bytes caption(bool automatic,uint32_t timestamp){Bytes b;for(auto v:{0x4730u,0xffffffffu,1u,hashName("IsSaveByAuto"),uint32_t(automatic),hashName("LastSaveTime_Lower"),timestamp,hashName("LastSaveTime_Upper"),0u,0xffffffffu})append(b,v);return b;}
static void require(bool ok,const char* message){if(!ok)throw Error(message);}
int main(){try{
    std::filesystem::create_directories("host-data/saves/0");
    auto original=document();writeFile("host-data/saves/0/game_data.sav",original);Bytes companion={'p','h','o','t','o'};writeFile("host-data/saves/0/caption.jpg",companion);
    Platform p;Profile profile{42,43,"Test",{}};Slot slot{"0","Test slot",{}};Save s(original);s.edit("rupees",[&]{s.integer(hashName("CurrentRupee"),0,50);});auto edited=s.bytes();
    auto backup=p.commit(profile,slot,s);require(readFile(backup+"/game_data.sav")==original,"Backup differs from original");require(readFile(backup+"/caption.jpg")==companion,"Companion was not backed up");require(readFile("host-data/saves/0/game_data.sav")==edited,"Wrong bytes committed");require(!s.dirty(),"Save was not accepted");
    s.edit("rupees",[&]{s.integer(hashName("CurrentRupee"),0,60);});writeFile("host-data/saves/0/game_data.sav",original);bool rejected=false;try{p.commit(profile,slot,s);}catch(const Error&){rejected=true;}
    require(rejected,"Stale save was accepted");require(readFile("host-data/saves/0/game_data.sav")==original,"Stale save overwrote live data");require(s.dirty(),"Failed save lost pending edits");
    auto again=p.open(profile,slot);require(again.bytes()==original,"Reopened document changed");
    // Match the root / ch_01 directory layout observed on a transferred Switch 2 save.
    for(const std::string& folder:{"6","7","ch_01/0","ch_01/1","ch_01/2","ch_01/3","ch_01/4","ch_01/5","unfamiliar/0","99"}){
        std::filesystem::create_directories("host-data/saves/"+folder);
        writeFile("host-data/saves/"+folder+"/game_data.sav",original);
        writeFile("host-data/saves/"+folder+"/caption.sav",caption(folder!="6"&&folder!="ch_01/0",folder=="ch_01/0"?1800000000:1700000000));
    }
    auto slots=p.slots(profile);
    auto find=[&](const std::string& folder)->Slot{auto it=std::find_if(slots.begin(),slots.end(),[&](const Slot& v){return v.folder==folder;});require(it!=slots.end(),"A save folder was missed");return *it;};
    require(slots.front().folder=="ch_01/0","Newest Profile 2 save was not sorted first");
    require(find("0").gameProfile==1&&!find("0").master,"Original manual save mislabeled");
    require(find("6").gameProfile==1&&find("6").master&&!find("6").automatic,"Master manual save mislabeled");
    require(find("7").master&&find("7").automatic,"Master autosave mislabeled");
    require(find("99").gameProfile==0&&!find("99").master,"Unknown numbered slot incorrectly assigned Master Mode");
    require(find("unfamiliar/0").gameProfile==0,"Unknown layout given a guessed profile");
    for(int i=0;i<6;++i){auto v=find("ch_01/"+std::to_string(i));require(v.gameProfile==2&&!v.master,"Profile 2 mistaken for Master Mode");require(v.automatic==(i!=0),"Profile 2 manual/auto label wrong");}
    auto second=find("ch_01/0");require(second.identity()=="Profile 2 · Manual save","Profile 2 identity not user-readable");
    auto secondSave=p.open(profile,second);secondSave.edit("rupees",[&]{secondSave.integer(hashName("CurrentRupee"),0,123);});auto secondBytes=secondSave.bytes();
    auto secondBackup=p.commit(profile,second,secondSave);
    require(readFile(secondBackup+"/game_data.sav")==original,"Profile 2 original not backed up");
    require(secondBackup.find("profile2-slotch_01%2F0")!=std::string::npos,"Nested profile backup identity ambiguous");
    require(readFile("host-data/saves/ch_01/0/game_data.sav")==secondBytes,"Profile 2 edit went to wrong slot");
    require(readFile("host-data/saves/0/game_data.sav")==original,"Profile 2 edit touched Profile 1");
    require(readFile("host-data/saves/ch_01/1/game_data.sav")==original,"Profile 2 edit touched another autosave");
    auto origin=readFile(secondBackup+"/zse-backup-origin.txt");require(std::string(origin.begin(),origin.end()).find("Original folder: ch_01/0")!=std::string::npos,"Backup lacks original location");
    for(const std::string& invalid:{"../0","/0","ch_01/../0","C:/save","ch_01\\0","ch_01//0","ch_01/"}){Slot bad;bad.folder=invalid;bool refused=false;try{p.open(profile,bad);}catch(const Error&){refused=true;}require(refused,"Unsafe save location accepted");}
    p.settings.theme=Theme::Oled;p.saveSettings();p.settings.theme=Theme::Light;p.loadSettings();require(p.settings.theme==Theme::Oled&&p.dark(),"OLED theme did not persist");
    std::cout<<"PASS: backup, companion backup, conflict refusal, transferred Profile 2 discovery and isolated writes, mode labels, backup identity, path validation, OLED persistence\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
